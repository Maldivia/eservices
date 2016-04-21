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
/* $Id: grep.c,v 1.5 2004/04/27 13:39:02 cure Exp $ */

#include <string.h>

#include "chanserv.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "errors.h"
#include "queue.h"
#include "log.h"

extern int chanserv_list_count;
extern chanserv_dbase_channel **chanserv_list;

/**************************************************************************************************
 * chanserv_grep
 **************************************************************************************************
 *   GREP <type> <mask>
 *   Type can be:
 *                name (channel name)
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
FUNC_COMMAND(chanserv_grep)
{
  char *type = getnext(params);
  char *mask = getnext(params);
  int no = 0; /* number of matches found! */

  /* access, no? then fuck off */
  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  /* if no arguments are given, return syntax */ 
  if (!type || !mask)
    return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);

  type = uppercase(type);

  if (!strcmp(type, "NAME"))
  {
    int i;

    com_message(sock, conf->cs->numeric, from->numeric, format, "Searching (%s) after %s.", type, mask);

    for (i = 0; i < chanserv_list_count; i++)
    {

      if (wildcard_compare(chanserv_list[i]->name, mask))
      {
        no++;
        com_message(sock, conf->cs->numeric, from->numeric, format, "  %-9s %-2lu %c%c %s", chanserv_list[i]->owner,
                    chanserv_list[i]->access_count, (chanserv_list[i]->flags & BITS_CHANSERV_EXPIRED)?'E':'.', 
                    (chanserv_list[i]->flags & BITS_CHANSERV_DISABLED)?'D':'.', chanserv_list[i]->name);
      }

    }
  }
  else
    return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);

  log_command(LOG_CHANSERV, from, "GREP", "%s %s", type, queue_escape_string(mask));
  return com_message(sock, conf->cs->numeric, from->numeric, format, "%d matches found for %s.", no, mask);
}
