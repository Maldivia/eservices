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
/* $Id: set.c,v 1.3 2004/04/27 13:39:02 cure Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chanserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"
#include "nickserv.h"

#define CHANSERV_SET_OK "%s on %s was successfully set to %s"

/* the external irc socket */
extern sock_info *irc;

/**************************************************************************************************
 * chanserv_set
 **************************************************************************************************
 *   SET <#channel> <option> <ON|OFF|setting>
 *      Turn an option for the channel on/off.
 *      <#channel> specifies the registered channel
 *      <option>   the option to change
 *      <ON|OFF|setting> the new value for the option
 **************************************************************************************************
 * Params:
 *   [IN] sock_info *sock    : The socket from which the data was recieved
 *   [IN] dbase_nicks *from  : Pointer to the user who issued this command
 *   [IN] char **params      : The parameters to the command (to be used with getnext and getrest)
 *   [IN] char *format       : The format the message should be returned (privmsg or notice)
 *   [IN] parser_command_data: *command_info : struct containing syntax, access-level etc.
 * Return:
 *  [OUT] int return         : return 0 if success, anything else will cause the services to stop
 **************************************************************************************************/
FUNC_COMMAND(chanserv_set)
{
  chanserv_dbase_channel *ch;
  /* Initialise variables with the parameters */
  char buf[2*BUFFER_SIZE], buf2[BUFFER_SIZE];
  char *chan   = getnext(params);
  char *option = getnext(params);
  char *args   = getrest(params);
  
  /* Is setting specified, ie is all the parameters present, if not, return syntax */
  if (!args) return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);

  /* check if the channel is registered */
  if (!(ch = chanserv_dbase_find_chan(chan))) return com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_CHANNEL_NOT_FOUND, chan);
  
  /* check to see if the channels is disabled */
  if (chanserv_dbase_disabled(ch)) return com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_CHANNEL_DISABLED);

  /* Does the user have access to this command */
  if (!chanserv_dbase_check_access(from->nickserv, ch, command_info->level))
    return ERROR_NO_ACCESS;

  option = uppercase(option);
  
  queue_escape_string_buf(ch->name, buf2);
  
  if (strcmp(option, "STRICTTOPIC") == 0)
  {
    int res;

    if ((res = set_is_ok(args)))
    {
      /* convert set_is_ok return, to C-style boolean, and set the flag for the channel */
      if (--res)
        ch->flags |= BITS_CHANSERV_STRICTTOPIC;
      else
        ch->flags &= ~BITS_CHANSERV_STRICTTOPIC;

        com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_SET_OK, option, chan, (ch->flags & BITS_CHANSERV_STRICTTOPIC)?"ON":"OFF");

      /* if strict topic enabled */
      if (ch->flags & BITS_CHANSERV_STRICTTOPIC)
      {
        /* if no topic set in chanserv, use the current on the channel */
        if (!ch->topic)
        {
          dbase_channels *db = channels_getinfo(-1, chan);
          if (!db)
            return com_message(sock, conf->cs->numeric, from->numeric, format, "Error, please report: %s:%d", __FILE__, __LINE__);

          /* if no topic on the channel, use a blank */
          if (!db->topic)
            channels_settopic(-1, chan, "");
          
          /* update the topic in chanserv to be the same as the topic on the channel */
          ch->topic = (char*)xrealloc(ch->topic, SIZEOF_CHAR * (strlen(db->topic)+1));
          strcpy(ch->topic, db->topic);
        }
        
        /* set the topic */
        com_send(irc, "%s T %s :%s\n", conf->cs->numeric, chan, ch->topic);
        channels_settopic(-1, chan, ch->topic);

        snprintf(buf, BUFFER_SIZE, "UPDATE chandata SET flags='%ld',topic='%s' WHERE name='%s'", ch->flags, queue_escape_string(ch->topic), buf2);
        queue_add(buf);

      }
    }
    else
      return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);
  }
  else if (strcmp(option, "MODE") == 0)
  {
/*    if (validmode(args))
    {
    }*/
  }
  else if (strcmp(option, "STRICTOPS") == 0)
  {
    int res;

    if ((res = set_is_ok(args)))
    {
      /* convert set_is_ok return, to C-style boolean, and set the flag for the channel */
      if (--res)
        ch->flags |= BITS_CHANSERV_STRICTOPS;
      else
        ch->flags &= ~BITS_CHANSERV_STRICTOPS;

        com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_SET_OK, option, chan, (ch->flags & BITS_CHANSERV_STRICTOPS)?"ON":"OFF");
        
      snprintf(buf, BUFFER_SIZE, "UPDATE chandata SET flags='%ld' WHERE name='%s'", ch->flags, buf2);
      queue_add(buf);
      
      /* if strict ops enabled */
      if (ch->flags & BITS_CHANSERV_STRICTOPS)
      {
        int i, count = 0;
        char buf[100], bufmode[10];
        dbase_channels *db = channels_getinfo(-1, chan);
        
        if (!db)
          return com_message(sock, conf->cs->numeric, from->numeric, format, "Error, please report: %s:%d", __FILE__, __LINE__);
        
        memset(buf, 0, 100);
        memset(bufmode, 0, 10);
        
        /* loop all users on the channel */
        for (i = 0; i < db->usercount; i++)
        {
          /* is this Chanserv ? */
          if (strncmp(db->users[i]->nick->numeric, conf->numeric, 2) == 0)
            continue;
          
          /* is the user opped */
          if (db->users[i]->mode & 2)
          {
            /* is the user authed */
            if (db->users[i]->nick->nickserv)
            {
              /* does the user have access on this channel */
              if (chanserv_dbase_has_access(db->users[i]->nick->nickserv->nick, ch))
                continue;
            }
            
            /* the user is not authed, or does not have access, deop */
            strcat(buf, db->users[i]->nick->numeric);
            strcat(buf, " ");
            bufmode[count++] = 'o';
            channels_usermode(-1, chan, "-o", db->users[i]->nick->numeric);
          }
          
          /* only deop 6 persons pr line */
          if (count >= 6)
          {
            com_send(irc, "%s M %s -%s %s\n", conf->cs->numeric, chan, bufmode, buf);
            memset(buf, 0, 100);
            memset(bufmode, 0, 10);
            count = 0;
          }
        }

        if (count)
          com_send(irc, "%s M %s -%s %s\n", conf->cs->numeric, chan, bufmode, buf);
      }
    }
    else
      return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);
  }
  else
    return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);


  log_command(LOG_CHANSERV, from, "SET", "%s %s", option, queue_escape_string(args)); 
      
  return 0;
}
/*
int validmode(char *mode)
{
}
*/
