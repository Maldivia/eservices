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
/* $Id: misc_func.h,v 1.10 2004/03/19 21:46:30 mr Exp $ */

#ifndef INC_MISC_FUNC_H
#define INC_MISC_FUNC_H

#include "time.h"
#include "dbase.h"
#include "chanserv.h"
#include "nickserv.h"

#define bitadd(test, bit) (test |= (1 << ((bit)%32)))
#define bitdel(test, bit) (test &= ~(1 << ((bit)%32)))
#define isbiton(test, bit) (test & (1 << ((bit)%32)))

typedef struct string_chain string_chain;

struct string_chain
{
  string_chain *next;
  char *str;
};

typedef union list_union
{
  chanserv_dbase_channel **chanserv;
  dbase_channels         **channels;
  nickserv_dbase_data    **nickserv;
  dbase_nicks            **nicks;
  void                   **generic;
} list_union;

typedef struct list_data
{
  list_union list;
  signed long size;
  signed long allocated;
} list_data;

list_data irc_list_create(void);
void irc_list_add(list_data *list, long position, void *elem);
void *irc_list_delete(list_data *list, long position);
void irc_list_clear(list_data *list);
void irc_list_move(list_data *list, long from, long to);

unsigned long tr_atoi(const char *s);
void strip_rn(char *str);
char *skipcolon(char **str);
char *getnext(char **str);
char *getrest(char **str);
char *uppercase(char *str);
unsigned long str64long(char *str);

void xfree(void *ptr);
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
int time_string_to_int(const char *str);
int wildcard_compare(const char *str, const char *wstr);
int match(const char *mask, const char *string);
const char *ircd_crypt(const char *key, const char *salt);
char *nick_password_crypt(nickserv_dbase_data *nick, const char *pass);
int password_compare(nickserv_dbase_data *nick, const char *pass);
const char *gtime(const time_t *clock);
const char *format_time(const time_t tm);

int set_is_ok(char *str);

void string_chain_traverse(void *ptr);
void string_chain_free(void *ptr);

#endif /* INC_MISC_FUNC_H */
