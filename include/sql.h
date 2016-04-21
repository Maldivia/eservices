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
/* $Id: sql.h,v 1.1 2004/01/17 14:10:00 cure Exp $ */

#include "setup.h"

#ifdef SQL_INTERFACE_ACTIVATED

#ifndef INC_SQL_H
#define INC_SQL_H

#include "dbase.h"
#include "server.h"

/* The main list of sql items waiting to be processed.
   Once an item has been processed it's thrown to its handler,
   which is supposed to take care of the data it returns.
 */

typedef struct sql_item 
{
  
  /* Pointer to the function which will handle the data.
     This function shield also call sql_free on the item. 
   */
  int (*handler)(struct sql_item *sql); 
  char *command; /* sql command to be executed */
  char *error; /* this string is set to the error if we encountered one */
  char ***buffer;  /* buffer which will contain the output */
  char **fields; /* field names :) */
  unsigned int *width; /* it's nice to know how wide the colums are so you can make it pretty */

  sock_info *sock; /* nice to know who to send data to, if any */
  dbase_nicks *from; 

  char format[6]; /* nice to know how to send it, if we want to send it */
  
  unsigned int num_rows;
  unsigned long num_fields;
  unsigned long rows_affected;

  struct sql_item *next;
} sql_item;


/* sql_free will free all allocated memory in
   a sql_item. 
 */
void sql_free(sql_item *item);
int sql_init(void);
int sql_add(const char *command, sock_info *sock, dbase_nicks *from, const char *format, int (*handler)(struct sql_item *sql));
void sql_wait_until_empty(void);

#endif
#endif
