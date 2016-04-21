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
/* $Id: netinfo.c,v 1.2 2004/01/16 02:17:37 cure Exp $ */

#include "errors.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "queue.h"
#include "log.h"
#include "setup.h"

#define OPERSERV_NETINFO_HEADER "Network information for %s:\n"

#define OPERSERV_NETINFO_BODY   "  Connected since: %s (%s) to %s:%d\n"\
                                "\n"\
                                "  Users online: %d Channels created: %d\n"\
                                "  Data send/received: %.1fkb/%.1fkb\n"\
                                "  Reg. users: %d Reg. Channels: %d\n"\
                                "\n"\
                                "  Compiled %s on %s"
#define OPERSERV_NETINFO_SQL    "  SQL interface is activated."
                                
extern time_t connected_since;
extern int    nickserv_list_count;
extern int    chanserv_list_count;
extern char   *build_date;
extern char   *os_name;

/**************************************************************************************************
 * operserv_netinfo
 **************************************************************************************************
 *   NETINFO 
 *   Displays information about servers and services.
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
FUNC_COMMAND(operserv_netinfo)
{
  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_NETINFO_HEADER, NETWORK_NAME);
  com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_NETINFO_BODY,
              gtime(&connected_since), format_time(time(0) - connected_since), conf->uplink, conf->port,
              nicks_getcount(), channels_getcount(),
              sock->datasend / 1024.0, sock->datarecv / 1024.0, 
              nickserv_list_count, chanserv_list_count,
              build_date, os_name
      );
#ifdef SQL_INTERFACE_ACTIVATED
  com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_NETINFO_SQL);
#endif

  log_command(LOG_OPERSERV, from, "NETINFO", "");

  return 0;
}
