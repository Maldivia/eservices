
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
/* $Id: sql.c,v 1.1 2004/01/17 02:17:21 cure Exp $ */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errors.h"
#include "operserv.h"
#include "misc_func.h"
#include "config.h"
#include "queue.h"
#include "log.h"
#include "dbase.h"
#include "sql.h"

#define OPERSERV_SQL_PROCESSING  "SQL EXEC: Processing query (%s). Please wait."
#define OPERSERV_SQL_EMPTY_SET   "SQL EXEC: Empty set."
#define OPERSERV_SQL_TRUNCATED   "SQL EXEC: Output truncated at %d rows."
#define OPERSERV_SQL_LIMIT       500

int handle_callback(sql_item *item);

/**************************************************************************************************
 * operserv_sql
 **************************************************************************************************
 *   SQL 
 *   Interface to the mysql databse.
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
FUNC_COMMAND(operserv_sql)
{
  char *sql_opt     = getnext(params);
  char *sql_command = getrest(params);

  if (!operserv_have_access(from->nickserv->flags, command_info->flags)) return ERROR_NO_ACCESS;

  if (!sql_opt || !sql_command) 
    return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);

  sql_opt = uppercase(sql_opt);

  if (!strcmp(sql_opt, "EXEC"))
  {
    com_message(sock, conf->os->numeric, from->numeric, format, OPERSERV_SQL_PROCESSING, sql_command);
    sql_add(sql_command, sock, from, format, handle_callback);
  }
  else
    return com_message(sock, conf->os->numeric, from->numeric, format, command_info->syntax);

  log_command(LOG_OPERSERV, from, "SQL", "%s %s", sql_opt, queue_escape_string(sql_command));

  return 0;
}

int handle_callback(sql_item *item)
{
  int i, j, width = 0;
  char *line, *delimiter;

  debug_out("handle_callback in operserv_sql called.\n");

  if ((item->rows_affected == ~0) && item->error)
    return com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, "MYSQL EXEC: %s", item->error);

  if (item->rows_affected != ~0)
    return com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, "Query OK, %lu rows affected.", item->rows_affected);

  if (!item->num_rows)
    return com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, OPERSERV_SQL_EMPTY_SET);



  for (i = 0; i < item->num_fields; i++) 
    width += item->width[i]; 

  width += ((item->num_fields*3) + 1);

  delimiter = (char *)xmalloc(width+1);
  line      = (char *)xmalloc(width+1);

  delimiter[0] = '\0';
  line[0] = '\0';

  for (i = 0; i < item->num_fields; i++)
  {
    strcat(delimiter, "+");
    for (j = 0; j < item->width[i]+2; j++)
      strcat(delimiter, "-");
  }
  strcat(delimiter, "+");

  com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, delimiter);

  line[0] = '\0';
  strcat(line, "| ");
  for (j = 0; j < item->num_fields; j++)
  {
    sprintf(line+strlen(line), "%-*s | ", item->width[j], item->fields[j]);
  }

  com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, line);

  com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, delimiter);

  for (i = 0; i < item->num_rows; i++)
  {
    if (i > OPERSERV_SQL_LIMIT)
    {
      com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, OPERSERV_SQL_TRUNCATED, OPERSERV_SQL_LIMIT);
      break;
    }
    line[0] = '\0';
    strcat(line, "| ");
    for (j = 0; j < item->num_fields; j++)
    {
      sprintf(line+strlen(line), "%-*s | ", item->width[j], item->buffer[i][j]);
    }
    com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, line);
  }

  com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, delimiter);
  com_message(item->sock, conf->os->numeric, item->from->numeric, item->format, "%lu rows in set.", item->num_rows);

  return 0;
}
