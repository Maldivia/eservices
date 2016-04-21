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
/* $Id: chnick.c,v 1.3 2004/01/16 15:53:19 mr Exp $ */

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

#define NICKSERV_CHNICK_ALREADY_REGGED      "%s is a registered nick, failed to change the account name."

#define NICKSERV_CHNICK_NOT_ONLINE          "%s is not currently online and authed.\n"\
                                            "You cannot change a users account name without them being online!"

#define NICKSERV_CHNICK_USER_ONLINE         "A user with the nick %s is currently online, and this user is not authed to %s!\n"\
                                            "Cannot change the account name to a nick which is currently in use by another user!"

#define NICKSERV_CHNICK_OK                  "%s's account was successfully changed to %s.\n"

extern int nickserv_list_count;
extern nickserv_dbase_data **nickserv_list;

/**************************************************************************************************
 * nickserv_chnick
 **************************************************************************************************
 *   CHNICK <registered nickname> <not registered nickname>
 *   +R +O +N
 *     Force-changes the rnick for <rnick>
 *      <rnick>     = getnext-string
 *      <new rnick> = getnext-string
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
FUNC_COMMAND(nickserv_chnick)
{
  nickserv_dbase_data *data;
  dbase_nicks *nick;
  int index;
  char buf[BUFFER_SIZE], buf_who[BUFFER_SIZE], buf_to[BUFFER_SIZE];
  const char *who = getnext(params);
  const char *to = getrest(params);

  /* does the user have access to this command */
  if (!from->nickserv) return ERROR_NO_ACCESS;
  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  /* enough paramters */
  if (!to) return com_message(sock, conf->ns->numeric, from->numeric, format, command_info->syntax);

  /* is the nick regged */
  if (!(data = nickserv_dbase_find_nick(who))) return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_NOT_REGISTERED, who);

  /* is the new nick regged */
  if (nickserv_dbase_find_nick(to)) return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_CHNICK_ALREADY_REGGED, to);
  
  /* is the account we are changing online and authed */
  if (!data->entry) return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_CHNICK_NOT_ONLINE, who);

  if ((nick = nicks_getinfo(NULL, to, -1)))
    if (data->entry != nick)
      return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_CHNICK_USER_ONLINE, to, who);

  index = nickserv_dbase_internal_search(0, nickserv_list_count-1, who);

  /* remove from the array */
  memmove(&nickserv_list[index], &nickserv_list[index+1], (nickserv_list_count - index - 1) * SIZEOF_VOIDP);

  /* Find place to insert it */
  index = nickserv_dbase_internal_search(0, nickserv_list_count-1, to);
  index = (1 + index) * -1;
  
  /* rearrange array and insert */
  if (index < (nickserv_list_count - 1))
    memmove(&nickserv_list[index+1], &nickserv_list[index], (nickserv_list_count - index - 1) * SIZEOF_VOIDP);
  nickserv_list[index] = data;

  /* change data in memory */
  data->nick = (char*)xrealloc(data->nick, (sizeof(to)+1)*SIZEOF_CHAR);
  strcpy(data->nick, to);

  /* save it to the database */  
  queue_escape_string_buf(who, buf_who);
  queue_escape_string_buf(to, buf_to);
  
  snprintf(buf, BUFFER_SIZE, "UPDATE nickdata SET nick='%s'  WHERE nick='%s'", buf_to, buf_who);
  queue_add(buf);
  snprintf(buf, BUFFER_SIZE, "UPDATE chandata SET owner='%s' WHERE owner='%s'", buf_to, buf_who);
  queue_add(buf);
  snprintf(buf, BUFFER_SIZE, "UPDATE access   SET nick='%s'  WHERE nick='%s'", buf_to, buf_who);
  queue_add(buf);
  snprintf(buf, BUFFER_SIZE, "UPDATE comment  SET nick='%s'  WHERE nick='%s'", buf_to, buf_who);
  queue_add(buf);
  snprintf(buf, BUFFER_SIZE, "UPDATE notice   SET nick='%s'  WHERE nick='%s'", buf_to, buf_who);
  queue_add(buf);
  /* UPDATE log_? ??? */

  /* log the command */
  log_command(LOG_NICKSERV, from, "CHPASS", "%s %s", buf_who, buf_to);
  /* return a confirmation-string */
  return com_message(sock, conf->ns->numeric, from->numeric, format, NICKSERV_CHNICK_OK, who, to);
}
