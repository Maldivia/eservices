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
/* $Id: ignores.c,v 1.1 2004/05/10 06:18:44 cure Exp $ */

#include <stdio.h>

#include "operserv.h"
#include "misc_func.h"
#include "errors.h"
#include "config.h"
#include "queue.h"
#include "log.h"

/**************************************************************************************************
 * operserv_ignores
 **************************************************************************************************
 *   TRACE <user@host>
 *   Lists all users ignored by services.
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
FUNC_COMMAND(operserv_ignores)
{
  int cnt, i, no = 0;
  char buffer[BUFFER_SIZE*2]; /* Saving the userhost into this bitch */
  dbase_nicks *n;

  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  cnt = nicks_getcount();

  com_message(sock, conf->os->numeric, from->numeric, format, "Services is currently ignoring:"); 

  for (i = 0; i < cnt; i++)
  {
    n = nicks_getinfo(NULL, NULL, i);
    /* copying nick!username@host into buffer */
    snprintf(buffer, BUFFER_SIZE * 2, "%s!%s@%s", n->nick, n->username, n->host);

    if (n->ignored == 1)
    {
      com_message(sock, conf->os->numeric, from->numeric, format, "  %-10s (%s)", n->nick, buffer);
      no++;
    } 
  } 

  log_command(LOG_OPERSERV, from, "IGNORES", "");

  return com_message(sock, conf->os->numeric, from->numeric, format, "%d users ignored.", cnt); 
}
