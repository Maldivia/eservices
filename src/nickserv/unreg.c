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
/* $Id: unreg.c,v 1.3 2004/03/19 21:53:06 mr Exp $ */

#include <string.h>

#include "nickserv.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"

#define NICKSERV_UNREG_OK                   "%s has been removed successfully."
#define NICKSERV_UNREG_NO_SUCH_USER         "%s is not a registered nickname."
#define NICKSERV_UNREG_NO_SELF              "You cannot use unreg to drop your own nick. Use drop instead."
#define  NICKSERV_NO_UNREG_OPER             "You cannot use unreg to drop an oper. Opers must be deopered first."

/**************************************************************************************************
 * nickserv_unreg
 **************************************************************************************************
 *   UNREG <rnick> FORCE
 *      +R +O +N
 *      Forcefully drops an account. 
 *      <rnick> = getnext-string
 *      FORCE   = getnext-string
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
FUNC_COMMAND(nickserv_unreg)
{
  char *nick = getnext(params);
  char *confirm = getnext(params);
  nickserv_dbase_data *data;

  /* if user not authed, return error */
  if (!from->nickserv) return ERROR_NO_ACCESS;
  
  /* Does the user have oper-access to this command */
  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;
    
  /* enough parameters? */
  if (!confirm) return com_message(sock, conf->ns->numeric, from->numeric, format, command_info->syntax);
  
  /* correct confirm-string? */
  if (strcmp(confirm, "FORCE")) return com_message(sock, conf->ns->numeric, from->numeric, format, command_info->syntax);

  /* does the user exist */
  if (!(data = nickserv_dbase_find_nick(nick)))
    return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_UNREG_NO_SUCH_USER, nick);
    
  /* is the user specified this user */
  if (data == from->nickserv)
    return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_UNREG_NO_SELF);

  /* is the user an oper */
  if (data->flags & BITS_OPERSERV_OPER)
    return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_NO_UNREG_OPER);
  
  /* log the command */
  log_command(LOG_NICKSERV, from, "UNREG", "%s FORCE", queue_escape_string(data->nick));

  /* Inform the user that the nick is being dropped */
  com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_UNREG_OK, data->nick);

  /* remove the nick from the database */
  nickserv_dbase_unreg(data); 
  
  return 0;
}
