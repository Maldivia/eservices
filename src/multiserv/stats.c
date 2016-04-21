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
/* $Id: stats.c,v 1.4 2004/01/08 19:47:28 cure Exp $ */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "setup.h"
#include "multiserv.h"
#include "misc_func.h"
#include "log.h"
#include "config.h"

extern int chanserv_list_count;
extern int nickserv_list_count;
extern nickserv_dbase_data **nickserv_list;

/**************************************************************************************************
 * multiserv_stats
 **************************************************************************************************
 *   STATS <option>
 *     Show some statistics, depends on <option>
 *       B: shows bandwidth statistics (total sent/recieved to/from the server)
 *       D: statistics about the database (numbers of nicks/channels)
 *       O: a list of online opers
 *       <option> = getnext-string
 **************************************************************************************************
 * Params:
 *   [IN] sock_info *sock    : The socket from which the data was recieved
 *   [IN] dbase_nicks *from  : Pointer to the user who issued this command
 *   [IN] char **params      : The parameters to the command (to be used with getnext and getrest)
 *   [IN] char *format       : The format the message should be returned (privmsg or notice)
 * Return:
 *  [OUT] int return         : return 0 if success, anything else will cause the services to stop
 **************************************************************************************************/ 
FUNC_COMMAND(multiserv_stats)
{
  char p, *option = getnext(params);

  if (!option) return com_message(sock, conf->ms->numeric, from->numeric, format, command_info->syntax);
  
  p = toupper(option[0]);

  /* b = bandwidth stats etc. */
  if (p == 'B')
  {
    com_message(sock, conf->ms->numeric, from->numeric, format, "Bandwidth stats for %s", conf->host);
    com_message(sock, conf->ms->numeric, from->numeric, format, "\n");
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Send:            %.1f kilobytes", sock->datasend / 1024.0);
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Recieved:        %.1f kilobytes", sock->datarecv / 1024.0);
  }
  else if (p == 'D')
  {
    int i, count = 0;
    for (i = 0; i < nickserv_list_count; i++)
    {
      if (nickserv_list[i]->entry) count++;
    }
    
    com_message(sock, conf->ms->numeric, from->numeric, format, "Database stats for %s", conf->host);
    com_message(sock, conf->ms->numeric, from->numeric, format, "\n");
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Registered nicks:      %5d", nickserv_list_count);
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Online authed users:   %5d", count);
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Online users:          %5d", nicks_getcount());
    com_message(sock, conf->ms->numeric, from->numeric, format, "\n");
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Registered channels:   %5d", chanserv_list_count);
    com_message(sock, conf->ms->numeric, from->numeric, format, "   Current channel count: %5d", channels_getcount());
  }
  else if (p == 'O')
  {
    int i, count = nicks_getcount();
    char buf[BUFFER_SIZE], buf2[BUFFER_SIZE];
    
    strcpy(buf, "    ");
    
    com_message(sock, conf->ms->numeric, from->numeric, format, "Online opers:");
    for (i = 0; i < count; i++)
    {
      dbase_nicks *n = nicks_getinfo(NULL, NULL, i);
      if (isbiton(n->modes, 'o'-'a'))
      {
        snprintf(buf2, BUFFER_SIZE, "%12s ", n->nick);
        strcat(buf, buf2);
        
        if (strlen(buf) > 79)
        {
          com_message(sock, conf->ms->numeric, from->numeric, format, buf);
          strcpy(buf, "    ");
        }
      }
    }
    if (strlen(buf))
      com_message(sock, conf->ms->numeric, from->numeric, format, buf);
  }
  else
    return com_message(sock, conf->ms->numeric, from->numeric, format, command_info->syntax);
  
  log_command(LOG_MULTISERV, from, "STATS", "%c", p);

  return 0;
}
