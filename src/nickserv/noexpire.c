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
/* $Id: noexpire.c,v 1.3 2004/03/19 21:46:37 mr Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nickserv.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"

#define NICKSERV_NOEXPIRE_OK                  "Noexpire for %s is now set to %s."
                                              

/**************************************************************************************************
 * nickserv_noexpire
 **************************************************************************************************
 *   noexpire <registered nickname> [on|off]
 *   +R +O +N
 *     Toggles or sets the noexpire flag for <rnick>
 *      <rnick>    = getnext-string
 *      <setting>  = getnext-string
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
FUNC_COMMAND(nickserv_noexpire)
{
  nickserv_dbase_data *data;
  int res = 0;
  char buf[BUFFER_SIZE];
  char *who = getnext(params);
  char *para = getrest(params);

  /* does the user have access to this command */
  if (!from->nickserv) return ERROR_NO_ACCESS;
  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  /* enough paramters */
  if (!who) return com_message(sock, conf->ns->numeric, from->numeric, format, command_info->syntax);

  /* is the nick regged */
  if (!(data = nickserv_dbase_find_nick(who))) return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_NOT_REGISTERED, who);

  if (para)
  {
    if ((res = set_is_ok(para)))
    {
      if ((res-1)) nickserv_dbase_setbit(data, BITS_NICKSERV_NOEXPIRE, 1);
      else nickserv_dbase_removebit(data, BITS_NICKSERV_NOEXPIRE, 1);
    }
  }
  
  if (!res)
  {
    if (data->flags & BITS_NICKSERV_NOEXPIRE)
      nickserv_dbase_removebit(data, BITS_NICKSERV_NOEXPIRE, 1);
    else
      nickserv_dbase_setbit(data, BITS_NICKSERV_NOEXPIRE, 1);
  }

  /* log the command */
  strcpy(buf, queue_escape_string(who));
  log_command(LOG_NICKSERV, from, "NOEXPIRE", "%s %s", buf, (data->flags & BITS_NICKSERV_NOEXPIRE)?"ON":"OFF");
  
  /* return a confirmation-string */
  return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_NOEXPIRE_OK, who, (data->flags & BITS_NICKSERV_NOEXPIRE)?"ON":"OFF");
}
