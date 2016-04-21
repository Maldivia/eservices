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
/* $Id: db_server.h,v 1.3 2003/10/19 22:21:38 mr Exp $ */

#ifndef INCL_DB_SERVER_H
#define INCL_DB_SERVER_H

typedef struct dbase_server
{
  char numeric[3];
  char *name;
  char *desc;
  int children_count;
  unsigned long linktime;
  struct dbase_server **children;
  struct dbase_server *parent;
} dbase_server;

void server_add(const char *from, const char *numeric, const char *name, unsigned long linktime, const char *desc);
dbase_server *server_search(const char *numeric);
dbase_server *server_find(const char *numeric, dbase_server *root);
dbase_server *server_find_name(const char *name, dbase_server *root);
void server_remove(const char *name);
void server_free(dbase_server *server, int top);
void server_dump(dbase_server *root, int level);
#endif /* INCL_DB_SERVER_H */
