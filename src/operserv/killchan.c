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
/* $Id: killchan.c,v 1.1 2004/08/05 00:02:01 mr Exp $ */

#include <string.h>

#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"

extern sock_info *irc;

#define OPERSERV_KILLCHAN_DONE             "KillChan for %s complete, %d users killed."
#define OPERSERV_KILLCHAN_INVALID_CHANNEL  "The specified channel does not exist."

/**************************************************************************************************
 * operserv_killchan
 **************************************************************************************************
 *   KILLCHAN <#channel> <reason>
 *   Kill all non-opers in the specified channel
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
FUNC_COMMAND(operserv_killchan)
{
  char *channel = getnext(params);
  char *reason  = getrest(params);
  char buf[BUFFER_SIZE];
  dbase_channels *chan;
  int i;
  int killed = 0;

  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  if (!reason) return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);

  if (channel[0] != '#') return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);
  
  if (!(chan = channels_getinfo(-1, channel)))
    return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_KILLCHAN_INVALID_CHANNEL);


  for (i = 0; i < chan->usercount; i++)
  {
    dbase_nicks *ns = chan->users[i]->nick;

    if (ns->nickserv)
      if (ns->nickserv->flags & BITS_NICKSERV_OPER)
        continue; /* Skipping opers */

    if (isbiton(ns->modes, 'o'-'a'))
      continue; /* Skipping opers */
      
    com_send(irc, "%s D %s :%s %s\n", conf->cs->numeric, ns->numeric, conf->cs->nick, reason);
    nicks_remove(ns->numeric);
    killed++;
    i--;
  }
  strcpy(buf, queue_escape_string(channel));
  log_command(LOG_OPERSERV, from, "KILLCHAN", "%s %s", buf, queue_escape_string(reason));

  com_wallops(conf->os->numeric, "%s (%s!%s@%s) has issued a KillChan of %s, reason: %s\n", from->nickserv->nick, from->nick, from->username, from->host, channel, reason);
      
  return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_KILLCHAN_DONE, channel, killed);
}
