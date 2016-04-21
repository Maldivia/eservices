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
/* $Id: sql.c,v 1.2 2004/01/17 14:42:38 cure Exp $ */

#include "setup.h"

#ifdef SQL_INTERFACE_ACTIVATED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <unistd.h>
#include <pthread.h>
#include <mysql.h>
#include <errmsg.h>

#include "sql.h"
#include "misc_func.h"
#include "config.h"
#include "queue.h"
#include "log.h"
#include "errors.h"
#include "main.h"

pthread_mutex_t mutex;

sql_item *sql_next;
sql_item *sql_last; 

int sql_running = 1;

void *sql_handle_query(void *arg)
{
  struct timeval timeout;
  MYSQL mysql, *conn;
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;
  unsigned long *lengths;
  int i, j;
  my_bool reconnect = 1;

  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_COMPRESS, 0);
  mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);

  if (!(conn = mysql_real_connect(&mysql, conf->mysql->host, conf->mysql->username, conf->mysql->password, conf->mysql->database, conf->mysql->port, conf->mysql->unixsock, 0)))
  {
    /* Error occured while connecting to mysql server. */
//    printf("Sometihng is wrong here...\n");
    log_command(LOG_SERVICES, NULL, "", "mySQL error: %s", mysql_error(&mysql));
    quit_service(ERROR_DBASE_MYSQL_ERROR);
    return NULL;
  }

  while (sql_running)
  {
    if (sql_next)
    {
      struct sql_item *current;
      current = sql_next;

      pthread_mutex_lock(&mutex);

      if (sql_next == sql_last) sql_last = NULL;
      sql_next = sql_next->next;

      pthread_mutex_unlock(&mutex);

      current->error = NULL;

      if ((i = mysql_query(conn, current->command)))
      {
        /* Error occurred executing query */
        if (i == CR_SERVER_GONE_ERROR || i == CR_SERVER_LOST)
        {
          mysql_close(conn);
          mysql_close(&mysql);

          pthread_mutex_lock(&mutex);

          current->next = sql_next;
          sql_next = current;

          pthread_mutex_unlock(&mutex);

          conn = mysql_real_connect(&mysql, conf->mysql->host, conf->mysql->username, conf->mysql->password, conf->mysql->database, conf->mysql->port, conf->mysql->unixsock, 0);
        }
        else
        {
          current->error = (char *)xmalloc(strlen(mysql_error(&mysql))+1);         
          strcpy(current->error, mysql_error(&mysql));
          current->handler(current);
          sql_free(current);
        }
        continue;
      }

      if (!(result = mysql_store_result(conn)))
      {
        /* Error occured while retreiving data, or query returned no data: INSERT, UPDATE, or DELETE etc */
        if (mysql_field_count(conn) == 0)
        { 
          /* Query returned no data */
          current->rows_affected = mysql_affected_rows(conn);
          current->handler(current);
          sql_free(current);
          mysql_free_result(result);
          continue;
        }
        /* Error confirmed */
        current->error = (char *)xmalloc(strlen(mysql_error(&mysql))+1);         
        strcpy(current->error, mysql_error(&mysql));
        current->handler(current);
        sql_free(current);
        mysql_free_result(result);
        continue;
      }

      current->num_rows = mysql_num_rows(result);
      
      if (!current->num_rows)
      {
        /* returned an empty set */
        current->handler(current);
        sql_free(current);
        mysql_free_result(result);
        continue;
      }

      current->num_fields = mysql_num_fields(result);
      fields              = mysql_fetch_fields(result);

      /* now for the dirty work! */

      current->fields = (char **)xmalloc(current->num_fields * sizeof(char *));
      for (i = 0; i < current->num_fields; i++)
      {
        current->fields[i] = (char *)xmalloc(strlen(fields[i].name)+1);
        strcpy(current->fields[i], fields[i].name);
      }

      /* find the max width and store it in our structure. */
      current->width = (unsigned int *)xmalloc(current->num_fields * sizeof(unsigned int));
      for (i = 0; i < current->num_fields; i++)
      {
        if (fields[i].max_length > strlen(fields[i].name))
          current->width[i] = fields[i].max_length;
        else
          current->width[i] = strlen(fields[i].name);
      }

      /* allocate memory for our list */
      current->buffer = (char ***)xmalloc(current->num_rows * sizeof(char *));  
      for (i = 0; i < current->num_rows; i++)
        current->buffer[i] = (char **)xmalloc(current->num_fields * sizeof(char *));

      for (j = 0; (row = mysql_fetch_row(result)); j++)
      {
        lengths = mysql_fetch_lengths(result);

        for (i = 0; i < current->num_fields; i++)
        {
          if (row[i] == NULL)
          {
            current->buffer[j][i] = (char *)xmalloc(5);
            strcpy(current->buffer[j][i], "NULL");
            continue;
          }
          current->buffer[j][i] = (char *)xmalloc(lengths[i]+1);
          strcpy(current->buffer[j][i], row[i]);
        }
      } 
       
      current->handler(current);
      sql_free(current);
      mysql_free_result(result);
      /* end of dirty work */
    }
    else
    {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 100000;
      select(0, NULL, NULL, NULL, &timeout);
    }
  }
  pthread_mutex_destroy(&mutex);
  mysql_close(conn);
  mysql_close(&mysql);
  return NULL;
}

int sql_add(const char *command, sock_info *sock, dbase_nicks *from, const char *format, int (*handler)(struct sql_item *sql))
{
  struct sql_item *item;

  item = (sql_item *)xmalloc(sizeof(sql_item));
  memset(item, 0, sizeof(sql_item));
  item->next = NULL;

  item->command = (char *)xmalloc(strlen(command)+1);
  strcpy(item->command, command);

  item->sock = sock;
  item->from = from;

  item->rows_affected = -1;

  strcpy(item->format, format);
  item->handler = handler;

  pthread_mutex_lock(&mutex);

  if (!sql_next) sql_next = item;
  if (sql_last) sql_last->next = item;
  sql_last = item;

  pthread_mutex_unlock(&mutex);
  return 0;
}

int sql_init(void)
{
  pthread_t tread;
  pthread_mutex_init(&mutex, NULL);
	pthread_create(&tread, NULL, sql_handle_query, NULL);

	return 0;
}

void sql_wait_until_empty(void)
{
  struct timeval timeout;
  while (sql_next)
  {
    timeout.tv_sec  = 0;
    timeout.tv_usec = 100000;
    select(0, NULL, NULL, NULL, &timeout);
  }
  sql_running = 0;
}

void sql_free(sql_item *item)
{
  int i, j;

  xfree(item->command);
  xfree(item->error);

  for (i = 0; i < item->num_fields; i++)
  {
    xfree(item->fields[i]);
  }
  xfree(item->width);
  xfree(item->fields);

  for (i = 0; i < item->num_rows; i++)
  {
    for (j = 0; j < item->num_fields; j++)
      xfree(item->buffer[i][j]);
    xfree(item->buffer[i]);
  }
  xfree(item->buffer);

  xfree(item);
}

#endif
