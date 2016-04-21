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
/* $Id: gline.c,v 1.5 2004/08/05 00:02:33 mr Exp $ */

#include <string.h>

#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"

extern sock_info *irc;

#define OPERSERV_GLINE_ADDED      "Gline added for %s expiring in %s."
#define OPERSERV_GLINE_CHAN_ADDED "Gline added for %s expiring in %s (%d nicks glined, %d ignored)."
#define OPERSERV_GLINE_NO_OPER    "You cannot specify an oper as nickname for GLINE."

/**************************************************************************************************
 * operserv_gline
 **************************************************************************************************
 *   GLINE <user@host> <durance> <reason>
 *   Bans a user from the network 
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
FUNC_COMMAND(operserv_gline)
{
  char *userhost = getnext(params);
  char *secs = getnext(params);
  char *reason = getrest(params);
  char buf[BUFFER_SIZE];
  int durance;

  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  if (!reason) return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);
  
  durance = time_string_to_int(secs);
  if (durance < 1) return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);
  if (!operserv_have_access(from->nickserv->flags, BITS_OPERSERV_SERVICES_SUB_ADMIN))
    if (durance > 3600) durance = 3600; /* Max 1 hour for non-admins */

  if (userhost[0] == '#')
  {
    int i;
    int glined = 0, ignored = 0;

    dbase_channels *chan = channels_getinfo(-1, userhost);
    if (!chan)
      return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_INVALID_GLINE);

    for (i = 0; i < chan->usercount; i++)
    {
      dbase_nicks *ns = chan->users[i]->nick;

      if (ns->nickserv)
      {
        if (ns->nickserv->flags & BITS_NICKSERV_OPER)
        {
          ignored++;
          continue; /* Skipping opers */
        }
      }

      if (isbiton(ns->modes, 'o'-'a'))
      {
        ignored++;
        continue; /* Skipping opers */
      }
      
      strcpy(buf, "*@");
      strcat(buf, ns->host);        
      
      com_send(irc, "%s GL * +%s %d :%s\n", conf->numeric, buf, durance, reason);
      glined++;
    }
    com_wallops(conf->os->numeric, "%s (%s!%s@%s) has issued a G:Line for %s lasting %s, reason: %s\n", from->nickserv->nick, from->nick, from->username, from->host, userhost, format_time((time_t)durance), reason);
  
    strcpy(buf, queue_escape_string(userhost));
    log_command(LOG_OPERSERV, from, "GLINE", "%s %s %s", buf, secs, queue_escape_string(reason));
      
    return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_GLINE_CHAN_ADDED, userhost, format_time((time_t)durance), glined, ignored);
  }
  else if (!operserv_valid_gline(userhost))
  {
    dbase_nicks *ns;
    
    if ((ns = nicks_getinfo(NULL, userhost, -1)))
    {
      if (ns->nickserv)
        if (ns->nickserv->flags & BITS_NICKSERV_OPER)
          return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_GLINE_NO_OPER);

      if (isbiton(ns->modes, 'o'-'a'))
        return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_GLINE_NO_OPER);

      userhost = buf;
      strcpy(userhost, "*@");
      strcat(userhost, ns->host);        
    }
    else    
      return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_INVALID_GLINE);
  }
    
  if (!operserv_have_access(from->nickserv->flags, BITS_OPERSERV_SERVICES_SUB_ADMIN))
    if (durance > 3600) durance = 3600; /* Max 1 hour for non-admins */

  com_send(irc, "%s GL * +%s %d :%s\n", conf->numeric, userhost, durance, reason);
  com_wallops(conf->os->numeric, "%s (%s!%s@%s) has issued a G:Line for %s lasting %s, reason: %s\n", from->nickserv->nick, from->nick, from->username, from->host, userhost, format_time((time_t)durance), reason);
      
  strcpy(buf, queue_escape_string(userhost));
  log_command(LOG_OPERSERV, from, "GLINE", "%s %s %s", buf, secs, queue_escape_string(reason));
      
  return com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_GLINE_ADDED, userhost, format_time((time_t)durance));
}
