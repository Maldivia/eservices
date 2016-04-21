/****************************************************************************
* Exiled.net IRC Services                                                   *
* Copyright (C) 2002-2003  Michael Rasmussen <the_real@nerdheaven.dk>       *
*                          Morten Post <cure@nerdheaven.dk>                 *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License for more details.                              *
*                                                                           *
* You should have received a copy of the GNU General Public License         *
* along with this program; if not, write to the Free Software               *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*****************************************************************************/
/* $Id: server.c,v 1.14 2004/03/23 11:54:19 mr Exp $ */

#include "setup.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "server.h"
#include "config.h"
#include "misc_func.h"
#include "parser.h"
#include "errors.h"
#include "timer.h"
#include "p10.h"
#include "dcc.h"

extern sock_info *irc;

pthread_mutex_t sock_mutex;

int ping_send = 0;
int last_message = 0;

int com_sock_array_count = 0;
sock_info **com_sock_array = NULL;

/* Is there a server currently bursting ?
   If yes, stall timer events */
int com_burst = 0;

/*
 ****************************************************************************************
 * com_sock_create
 ****************************************************************************************
 *   Creates a socket of the specified type.
 *   If attach is true (!= 0), com_sock_attach() is automatically called
 ****************************************************************************************
 * Params:
 *   [IN]  sock_type type    : type of socket
 *   [IN]  int attach        : bool whether or not to call com_sock_attach
 * Return:
 *   [OUT] sock_info *return : the newly created socket, or NULL
 ****************************************************************************************
 */
sock_info *com_sock_create(sock_type type, int attach)
{
  sock_info *sock;
  
  if ((sock = (sock_info *) xmalloc(sizeof(sock_info))))
  {
    sock->type = type;
    sock->datarecv = 0;
    sock->datasend = 0;
    sock->sockfd   = INVALID_SOCKET;
    sock->from     = NULL;
    
    if (attach)
      com_sock_attach(sock);
  }

  return sock;
}

/*
 ****************************************************************************************
 * com_sock_attach
 ****************************************************************************************
 *   Adds the socket to the list of sockets
 ****************************************************************************************
 * Params:
 *   [IN]  sock_info *sock : The socket to add
 ****************************************************************************************
 */
void com_sock_attach(sock_info *sock)
{
  int i;
  /* if sock == NULL, invalid socket, return */
  if (!sock) return;
  
  pthread_mutex_lock(&sock_mutex); 
  
  /* if socket is in the list already, return */
  for (i = 0; i < com_sock_array_count; i++)
    if (com_sock_array[i] == sock) 
    {
      pthread_mutex_unlock(&sock_mutex);
      return;
    }
  
  /* expand the array and add the socket to it */
  com_sock_array = (sock_info **) xrealloc(com_sock_array, com_sock_array_count+1);
  com_sock_array[com_sock_array_count] = sock;
  memset(&sock->buffer, 0, BUFFER_SIZE);
  com_sock_array_count++;
  
  pthread_mutex_unlock(&sock_mutex);
}

/*
 ****************************************************************************************
 * com_connect
 ****************************************************************************************
 *   Connects a SERVER socket to the IRC server specifiec in the conf-file,
 *   and initializes the handshake (PASS/SERVER combo).
 ****************************************************************************************
 * Params:
 *   [IN]  sock_info *sock   : The socket which should be connected
 * Return:
 *   [OUT] sock_info *return : sock if connected, NULL otherwise
 ****************************************************************************************
 */
sock_info *com_connect(sock_info *sock)
{
  struct sockaddr_in dest;
  struct hostent *hostinfo;
  long t;
  
  /* is this a SERVER socket */
  if (sock->type != SOCK_SERVER) return NULL;

  /* Can I resolve the hostname specified in the conf-file */
  if ((hostinfo = gethostbyname(conf->uplink)) == NULL) return NULL;

  dest.sin_family = hostinfo->h_addrtype;
  dest.sin_port = htons(conf->port);

  dest.sin_addr = *((struct in_addr *)hostinfo->h_addr);
  memset(&(dest.sin_zero), 0, 8);

  /* Create the socket */
  if ((sock->sockfd = socket(hostinfo->h_addrtype, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    xfree(sock);
    return NULL;
  }

  /* if bind is specified in the conf-file, bin to the specified IP */
  if (conf->bind)
  {
    struct sockaddr_in virtual;

    memset(&virtual, 0, sizeof(virtual));

    virtual.sin_family = AF_INET;
    virtual.sin_addr.s_addr = INADDR_ANY;
   
    virtual.sin_addr.s_addr = inet_addr(conf->bind);
  
    if (virtual.sin_addr.s_addr == INADDR_NONE)
      virtual.sin_addr.s_addr = INADDR_ANY;

    bind(sock->sockfd, (struct sockaddr *)&virtual, sizeof(virtual));
  }

  /* Connect to the specified host */
  if (connect(sock->sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)))
  {
    xfree(sock);
    return NULL;
  }

  /* Shake hands with the server */
  t = time(0);
  com_send(irc, "PASS :%s\r\n", conf->pass);
  com_send(irc, "SERVER %s 1 %ld %ld J10 %sKA] + :%s IRC Services\r\n", conf->host, t, t, conf->numeric, NETWORK_NAME);
  conf->starttime = t;
  com_burst = 1;
 
  /* Everything went well, return sock as confirmation */
  return sock;
}

/*
 ****************************************************************************************
 * com_mainloop
 ****************************************************************************************
 *   This is the main loop, which select's over the sockets, and parses the input.
 ****************************************************************************************
 * Return:
 *   [OUT] int return : The exit-code
 ****************************************************************************************
 */
int com_mainloop(void)
{
  int sel = 0, i, res, nfds = 0;
  fd_set rset;
  struct timeval timeout;
  
  last_message = time(0);

  while(1)
  {
    timeout.tv_sec =  1;
    timeout.tv_usec = 0;

    FD_ZERO(&rset);
    nfds = 0;
    
    pthread_mutex_lock(&sock_mutex);
    for (i = 0; i < com_sock_array_count; i++)
    {
      if (com_sock_array[i]->sockfd == INVALID_SOCKET) continue;
      FD_SET(com_sock_array[i]->sockfd, &rset);
      if (com_sock_array[i]->sockfd > nfds) nfds = com_sock_array[i]->sockfd;
    }
    pthread_mutex_unlock(&sock_mutex);

    nfds++;

    sel = select(nfds, &rset, NULL, NULL, &timeout);

    if (sel > 0)
    {
      pthread_mutex_lock(&sock_mutex);
      for (i = 0; i < com_sock_array_count; i++)
      {
        if (FD_ISSET(com_sock_array[i]->sockfd, &rset))
        {
          if ((res = com_recieve(com_sock_array[i])))
          {
            if (com_sock_array[i]->type == SOCK_SERVER) 
            {
              pthread_mutex_unlock(&sock_mutex);
              return res;
            }
            else
              com_free(com_sock_array[i--]);
          }
          else if (com_sock_array[i]->type == SOCK_SERVER)
          {
            last_message = time(0);
          }
        }
      }
      pthread_mutex_unlock(&sock_mutex);
    }
    else if (sel < 0)
    {
      /* if dcc / telnet close/free */
      /* if server - try to free all and reconnect */
    }
    if (!com_burst) timer_check();
    
    if ((time(0) > (last_message + PING_WAIT)) && (!ping_send))
    {
      com_send(irc, "%s G :%s\n", conf->numeric, conf->host);
      ping_send = time(0);      
    }
    else if ((ping_send) && (time(0) > (ping_send + PING_WAIT)))
    {
      com_send(irc, "%s SQ %s %lu :ping timeout\n", conf->os->numeric, conf->host, conf->starttime);
      return ERROR_PING_TIMEOUT;
    }    
  }

  return 0;
}

/*
 ****************************************************************************************
 * com_free
 ****************************************************************************************
 *   Disconnects a socket, and free's all associated memory
 ****************************************************************************************
 * Params:
     [IN]  sock_info *sock : The socket to close and free
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_free(sock_info *sock)
{
  int i;
  
  pthread_mutex_lock(&sock_mutex);  
  for (i = 0; i < com_sock_array_count; i++)
  {
    if (com_sock_array[i] == sock)
    {
      
      if (sock->type == SOCK_DCC)
        dcc_on_free(sock);
      
      close(sock->sockfd);
      
      memmove(&com_sock_array[i], &com_sock_array[i+1], (com_sock_array_count - i - 1) * sizeof(sock_info*));
      com_sock_array = (sock_info**)xrealloc(com_sock_array, (com_sock_array_count-1) * sizeof(sock_info*));
      com_sock_array_count--;
      
      debug_out("Closing socket, buffer: %s\n", sock->buffer);

      xfree(sock);
      sock = NULL;
      
      pthread_mutex_unlock(&sock_mutex);
      
      return 0;
    }
  }
  pthread_mutex_unlock(&sock_mutex);
  
  /* Free a socket not attached to the list */
  if (sock)
  {
    close(sock->sockfd);
    xfree(sock);
  }  
  
  return 0;
}

/*
 ****************************************************************************************
 * com_free_all
 ****************************************************************************************
 *   Disconnects all sockets, and free's all associated memory
 ****************************************************************************************
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_free_all(void)
{
  int i;
  pthread_mutex_lock(&sock_mutex);  
  for (i = 0; i < com_sock_array_count; i++)
  {
    close(com_sock_array[i]->sockfd);
    xfree(com_sock_array[i]);
  }
  xfree(com_sock_array);
  com_sock_array = NULL;
  com_sock_array_count = 0;
  pthread_mutex_unlock(&sock_mutex);
  return 0;
}

/*
 ****************************************************************************************
 * com_recieve
 ****************************************************************************************
 *   Recieve data from the socket into it's buffer
 ****************************************************************************************
 * Params:
 *   [IN]  sock_info *sock : The socket to handle
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_recieve(sock_info *sock)
{
  char in[BUFFER_SIZE], *p;
  int recv_len = 0, res;
  int pre = strlen(sock->buffer);

  if (pre == BUFFER_SIZE)
  {
    /* ARGH!! Our buffer is full, and all this in one line...
       Add a newline, so the buffer can be flushed
    */
    sock->buffer[BUFFER_SIZE-1] = '\n';
  }
  else
  {
    /* Read data from the socket into a buffer */
    recv_len = read(sock->sockfd, in, sizeof(in) - pre - 1);

    if (recv_len > 0)
      sock->datarecv += recv_len;
    else
      return  ERROR_COM_CONNECTION_LOST;
  
    in[recv_len] = '\0';

    /* Add the read data to already recieved unparsed data */
    strcat(sock->buffer, in);
    pre = strlen(sock->buffer);
    memset(sock->buffer+pre, 0, BUFFER_SIZE - pre);
  }
  
  /* parse the data in the buffer, on line at a time */
  while ((p = strchr(sock->buffer, '\xa')))
  {
    /* remove the newline from the buffer, and the linefeed if exists */
    *p-- = '\0';
    if (*p == '\xd') *p = '\0';
    p++; p++;
    
    /* Determine which parser should parse the data */    
    switch (sock->type)
    {
      case SOCK_SERVER:
        debug_out("[RAW/IN]  %s\n", sock->buffer);
        if ((res = parser_p10(sock, sock->buffer))) return res;
        break;
      case SOCK_TELNET:
        /* Telnet not implemented yet */
        /* 
        parser_telnel(sock);
        break;        
        */
      case SOCK_DCC:
        debug_out("[DCC/IN]  [%s] %s\n", sock->from->nickserv->nick, sock->buffer);
        parser_dcc(sock, sock->buffer);
        break;
    }
    /* Clear the just parsed data from the buffer */
    memmove(sock->buffer, p, BUFFER_SIZE - (p - sock->buffer));
  }
  return 0;
}

/*
 ****************************************************************************************
 * com_send
 ****************************************************************************************
 *   Send data through the specified socket, using printf style
 ****************************************************************************************
 * Params:
 *   [IN]  sock_info *sock : The socket to send data through
 *   [IN]  char *string    : Format-string as printf
 *   [IN]  ...             : Parameters to the format-string
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_send(sock_info *sock, const char *string, ...)
{
  char buffer[BUFFER_SIZE];
  va_list ag;

  if (!string) return 0;
  if (!sock) return 0;

  /* Parse the format string */
  va_start(ag, string);
  vsnprintf(buffer, BUFFER_SIZE, string, ag);
  va_end(ag);

  if (sock->type == SOCK_SERVER)
    debug_out("[RAW/OUT] %s", buffer);
  else if (sock->type == SOCK_DCC)
    debug_out("[DCC/OUT] [%s] %s", sock->from->nickserv->nick, buffer);

  /* send the data */
  sock->datasend += send(sock->sockfd, buffer, strlen(buffer), 0);
  return 0;
}

/*
 ****************************************************************************************
 * com_message
 ****************************************************************************************
 *   Send a message to a user through the specified socket
 ****************************************************************************************
 * Params:
 *   [IN]  sock_info *sock : The socket to send data through
 *   [IN]  char *from      : The sender of the message (numeric)
 *   [IN]  char *to        : The reciver of the message (numeric)
 *   [IN]  char *format    : The format of the message (privmsg og notice)
 *   [IN]  char *string    : Format-string as printf
 *   [IN]  ...             : Parameters to the format-string
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_message(sock_info *sock, const char *from, const char *to, const char *format, const char *string, ...)
{
  char buffer[2*BUFFER_SIZE], *buf, *p;
  va_list ag;

  if (!string) return 0;

  memset(buffer, 0, sizeof(buffer));
  
  /* Parse the format-string */
  va_start(ag, string);
  vsnprintf(buffer, 2*BUFFER_SIZE, string, ag);
  va_end(ag);

  buf = buffer;

  /* Parse one line at a time */
  while ((p = strchr(buf, '\xa')))
  {
    *p++ = '\0';
    if (sock->type == SOCK_SERVER)
      com_send(sock, "%s %s %s :%s\n", from, format, to, buf);
    else
      com_send(sock, "%s\n", buf);
    buf = p;
  }
  if (*buf)
  {
    if (sock->type == SOCK_SERVER)
      com_send(sock, "%s %s %s :%s\n", from, format, to, buf);
    else
      com_send(sock, "%s\n", buf);
  }

  return 0;
}

/*
 ****************************************************************************************
 * com_wallops
 ****************************************************************************************
 *   Send a wallops
 ****************************************************************************************
 * Params:
 *   [IN]  char *sender    : The sender of the wallops (numeric)
 *   [IN]  char *string    : Format-string as printf
 *   [IN]  ...             : Parameters to the format-string
 * Return:
 *   [OUT] int return      : ...
 ****************************************************************************************
 */
int com_wallops(const char *sender, const char *string, ...)
{
  char buffer[BUFFER_SIZE], *p;
  va_list ag;

  if (!string) return 0;
  if (!irc) return 0;

  /* parse the format-string */
  va_start(ag, string);
  vsnprintf(buffer, BUFFER_SIZE, string, ag);
  va_end(ag);
  
  p = buffer;
  
  /* make the p10_wallops parse the function, to make it show in logs and DCC interface etc */
  p10_wallops(irc, sender, &p);
  /* Send it to the server */
  com_send(irc, "%s WA :%s\n", sender, buffer);

  return 0;
}
