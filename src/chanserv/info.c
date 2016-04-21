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
/* $Id: info.c,v 1.6 2004/04/27 13:39:02 cure Exp $ */
#include "setup.h"

#include <string.h>

#include "channels.h"
#include "chanserv.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "queue.h"
#include "log.h"

#define CHANSERV_ACCESS_LIST_HEAD      "Access list for %s:\n"
#define CHANSERV_ACCESS_LIST_ENTRY     "  %c%3d %s"

/**************************************************************************************************
 * chanserv_info
 **************************************************************************************************
 *   INFO <#channel>
 *     Returns information about <#channel>, it's owner etc.
 *
 *   INFO <#channel> [x]
 *     +D
 *     Returns information about <#channel> etc, including a complete list of all users
 *     currently in the channel
 *     <#channel> = getnext-string
 *     [x]        = getnext-string
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
FUNC_COMMAND(chanserv_info)
{
  int i, j;
  dbase_channels *ch;
  chanserv_dbase_channel *data;
  char *chan = getnext(params);

  if (!chan) return com_message(sock, conf->cs->numeric, from->numeric, format, command_info->syntax);

  data = chanserv_dbase_find_chan(chan);
  ch = channels_getinfo(-1, chan);

  if (data)
  {  
    com_message(sock, conf->cs->numeric, from->numeric, format, "Information about %s:", chan);
    com_message(sock, conf->cs->numeric, from->numeric, format, "  Status.....: %s", ((data->flags & BITS_CHANSERV_DISABLED)?"Disabled":"Active"));
    com_message(sock, conf->cs->numeric, from->numeric, format, "  Owner......: %s", data->owner);

    if ((chanserv_dbase_check_access(from->nickserv, data, CHANSERV_LEVEL_SET)) ||
       (operserv_have_access(from->nickserv->flags, BITS_OPERSERV_CS_OPER)))
    {
      if (ch)
      {
        char buf[BUFFER_SIZE];

        com_message(sock, conf->cs->numeric, from->numeric, format, "  Expired....: %s", (data->flags & BITS_CHANSERV_EXPIRED)?"YES":"NO");
        com_message(sock, conf->cs->numeric, from->numeric, format, "  No Expire..: %s", (data->flags & BITS_CHANSERV_NOEXPIRE)?"ON":"OFF");
        com_message(sock, conf->cs->numeric, from->numeric, format, "  Stricttopic: %s", (data->flags & BITS_CHANSERV_STRICTTOPIC)?"ON":"OFF");
        com_message(sock, conf->cs->numeric, from->numeric, format, "  Strictops..: %s", (data->flags & BITS_CHANSERV_STRICTOPS)?"ON":"OFF");
        com_message(sock, conf->cs->numeric, from->numeric, format, "  Laston.....: %s", gtime((time_t*)&data->lastlogin));
        com_message(sock, conf->cs->numeric, from->numeric, format, "  Comments...: %lu", data->comment_count);
    
        if (ch->topic) 
          com_message(sock, conf->cs->numeric, from->numeric, format, "  Topic......: %s", ch->topic);
    
        strcpy(buf, gtime((time_t *)&ch->createtime));
        com_message(sock, conf->cs->numeric, from->numeric, format, "  Created....: %s", buf);
        j = 0;
        buf[0] = '\0';
        for (i = 0; i < 32; i++)
        {
          if (isbiton(ch->modes, i))
          {
            buf[j++] = 'a'+i;
            buf[j] = '\0';
          }
        }
        if (buf[0])
          com_message(sock, conf->cs->numeric, from->numeric, format, "  Mode.......: %s", buf);
        if (ch->limit) 
          com_message(sock, conf->cs->numeric, from->numeric, format, "  Limit......: %d", ch->limit);
        if (ch->key) 
          com_message(sock, conf->cs->numeric, from->numeric, format, "  Key........: %s", ch->key);
      }
    }
    
    if (operserv_have_access(from->nickserv->flags, BITS_OPERSERV_CS_OPER))
    {
      int i;
      com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_ACCESS_LIST_HEAD, chan);
      for (i = 0; i < data->access_count; i++)
        com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_ACCESS_LIST_ENTRY, data->access[i]->autoop?'@':' ', data->access[i]->level, data->access[i]->nick->nick);
    }
  }
  else
    com_message(sock, conf->cs->numeric, from->numeric, format, CHANSERV_CHANNEL_NOT_FOUND, chan);

  if (ch && (operserv_have_access(from->nickserv->flags, BITS_OPERSERV_CS_OPER)))
  {
    if (ch->bancount)
    {
      com_message(sock, conf->cs->numeric, from->numeric, format, "  Bans.....: %s", ch->bans[0]);
      for (i = 1; i < ch->bancount; i++)
        com_message(sock, conf->cs->numeric, from->numeric, format, "             %s", ch->bans[i]);
    }
    if (ch->usercount)
    {
      char *modestr[4] = {"  ", " +", " @", "+@"};
      com_message(sock, conf->cs->numeric, from->numeric, format, "  Users....: %s%s", modestr[ch->users[0]->mode], ch->users[0]->nick->nick);
      for (i = 1; i < ch->usercount; i++)
        com_message(sock, conf->cs->numeric, from->numeric, format, "             %s%s", modestr[ch->users[i]->mode], ch->users[i]->nick->nick);
    }
  }

  log_command(LOG_CHANSERV, from, "INFO", queue_escape_string(chan));
  
  return 0;
}
