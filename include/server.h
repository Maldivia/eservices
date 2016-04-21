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
/* $Id: server.h,v 1.6 2004/01/08 19:46:39 cure Exp $ */

#ifndef INC_SERVER_H
#define INC_SERVER_H

#include "setup.h"

#define INVALID_SOCKET -1

typedef struct sock_info sock_info;

#include "dbase.h"

typedef enum sock_type
{
  SOCK_SERVER,
  SOCK_DCC,
  SOCK_TELNET
} sock_type;

struct sock_info
{
  sock_type type;
  int sockfd;
  unsigned long datarecv;
  unsigned long datasend;
  dbase_nicks *from;
  char buffer[BUFFER_SIZE];
};

sock_info *com_sock_create(sock_type type, int attach);
void com_sock_attach(sock_info *sock);
sock_info *com_connect(sock_info *irc);
int com_mainloop(void);
int com_recieve(sock_info *sock);
int com_send(sock_info *sock, const char *string, ...);
int com_message(sock_info *sock, const char *from, const char *to, const char *format, const char *string, ...);
int com_free(sock_info *sock);
int com_free_all(void);
int com_wallops(const char *sender, const char *string, ...);

#endif /* INC_SERVER_H */
