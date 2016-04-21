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
/* $Id: misc_func.c,v 1.14 2004/03/19 21:46:35 mr Exp $ */

#include "setup.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Make Linux happy */
#define __USE_XOPEN

#include <unistd.h>

/* For systems which defines crypt() in crypt.h instead of unistd.h */
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#endif

#include "misc_func.h"
#include "server.h"
#include "dbase.h"
#include "queue.h"

/* declare the external irc socket */
extern sock_info *irc;

static char gtime_str[30];
static char format_time_str[BUFFER_SIZE]; /* used by the format_time() function */

const char *ircd_crypt(const char *key, const char *salt)
{
  if ((!key) || (!salt)) return NULL;
  return crypt(key, salt);
}

void tohex(char *dst, const char *src, unsigned int length)
{
  char hex[17] = "0123456789abcdef";
  unsigned int i;
  for (i = 0; i < length; i++)
  {
    dst[2*i]     = hex[(src[i] & 0xf0) >> 4];
    dst[2*i + 1] = hex[(src[i] & 0x0f)];
  }
  dst[2*i] = 0;
}

char *nick_password_crypt(nickserv_dbase_data *nick, const char *pass)
{
  char buf[SHA_DIGEST_LENGTH * 2 + 1], buf2[SHA_DIGEST_LENGTH * 2 + 1];
  char mybuf[BUFFER_SIZE];
#ifdef HAVE_OPENSSL_SHA_H
  SHA1(pass, strlen(pass), buf2);
  tohex(buf, buf2, SHA_DIGEST_LENGTH);
#else
  strcpy(buf, crypt(pass, nick->userhost));
#endif
  nick->password = (char*)xrealloc(nick->password, strlen(buf)+1);
  strcpy(nick->password, buf);
  queue_escape_string_buf(nick->nick, buf2);

  snprintf(mybuf, BUFFER_SIZE, "UPDATE nickdata SET password='%s' where nick='%s'", nick->password, buf2);
  queue_add(mybuf);
  
  return nick->password;
}

int password_compare(nickserv_dbase_data *nick, const char *pass)
{
#ifdef HAVE_OPENSSL_SHA_H
  if (strlen(nick->password) < 25)
  {
    int res = strcmp(nick->password, crypt(pass, nick->password));
    if (!res)
    {
      nick_password_crypt(nick, pass);
    }
    return res;
  }
  else
  {
    char buf[SHA_DIGEST_LENGTH * 2 + 1], buf2[SHA_DIGEST_LENGTH];
    SHA1(pass, strlen(pass), buf2);
    tohex(buf, buf2, SHA_DIGEST_LENGTH);
    return strcasecmp(nick->password, buf);
  }
#endif
  return strcmp(nick->password, crypt(pass, nick->password));
}

void strip_rn(char *str)
{
  int i = strlen(str);
  while (i)
  {
    i--;
    if (str[i] == '\r') str[i] = '\0';
    if (str[i] == '\n') str[i] = '\0';
    if (str[i] != '\0') break;
  }
}

unsigned long tr_atoi(const char *s)
{
  const char *p = s;
  if (!s) return 0;
  while(*p)
    if ((*p > '9') || (*p++ < '0'))
      return 0;
  return strtoul(s, (char**)NULL, 10);
}

char *skipcolon(char **str)
{
  char *res = *str;
  if (!str) return NULL;
  if (!*str) return NULL;
  if (!**str) return NULL;
  if (*res == ':') res++;
  while (*res == ' ') res++;
  *str = res;
  return res;
}

char *getrest(char **str)
{
  char *res = *str;
  if (!str) return NULL;
  if (!*str) return NULL;
  if (!**str) return NULL;
  *str = NULL;
  if (res[0] == ':') res++;
  return res;
}

char *getnext(char **str)
{
  char *tmp, *res;
  if (!str) return NULL;
  if (!*str) return NULL;
  if (!**str) return NULL;

  res = *str;
  if (res[0] == ':') return getrest(str);

  if ((tmp = strchr(*str, ' ')))
  {
    *tmp++ = '\0';
    while (*tmp == ' ')
    {
      tmp++;
    }
    *str = tmp;
    return res;
  }
  return getrest(str);
}

unsigned long str64long(char *str)
{
  short base64[75] = \
    {52, 53, 54, 55, 56, 57, 58, 59, 60, 61,100,100,100,100,100,100,100, \
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, \
     17, 18, 19, 20, 21, 22, 23, 24, 25, 62,100, 63,100,100,100, 26, 27, \
     28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, \
     45, 46, 47, 48, 49, 50, 51};
  long bases[6] = {0x1, 0x40, 0x1000, 0x40000, 0x1000000, 0x40000000};
  unsigned int i;
  unsigned long res = 0;
  if (!str) return 0;
  for (i=0; i < strlen(str); i++)
    res += ( bases[strlen(str)-1-i] * base64[str[i] - '0'] );
  return res;
}

void xfree(void *ptr)
{
  if (ptr) free(ptr);
}

void *xmalloc(size_t size)
{
  if (size > 0 && size < 4) size = 4;
  return size ? malloc(size) : NULL;
}

void *xcalloc(size_t count, size_t size)
{
  return count * size ? calloc(count, size) : NULL;
}

void *xrealloc(void *ptr, size_t size)
{
  if (size > 0 && size < 4) size = 4;
  if (size == 0) xfree(ptr);
  return size ? realloc(ptr, size) : NULL;
} 

int time_string_to_int(const char *str)
{
  const char *p = str;
  char buf[BUFFER_SIZE];
  int pos = 0, tid = 0, i;
  
  if (!str) return -1;
  if (str[0] == '%')
  {
    p++;
    while (*p)
    {
      if ((*p >= '0') && (*p <= '9'))
      {
        buf[pos++] = *p;
        buf[pos] = 0;
      }
      else if ((*p == 's') || (*p == 'S'))
      {
        if (!(i = tr_atoi(buf))) return -1;
        tid += i;
        pos = 0;
        buf[0] = 0;
      }
      else if ((*p == 'm') || (*p == 'M'))
      {
        if (!(i = tr_atoi(buf))) return -1;
        tid += (i * (60));
        pos = 0;
        buf[0] = 0;
      }
      else if ((*p == 'h') || (*p == 'H'))
      {
        if (!(i = tr_atoi(buf))) return -1;
        tid += (i * (60*60));
        pos = 0;
        buf[0] = 0;
      }
      else if ((*p == 'd') || (*p == 'D'))
      {
        if (!(i = tr_atoi(buf))) return -1;
        tid += (i * (60*60*24));
        pos = 0;
        buf[0] = 0;
      }
      else return -1;
      p++;
    }
  }
  else
  {
    tid = tr_atoi(str);
    if (tid == 0) return -1;
  }
  return tid;
}

int wildcard_compare(const char *str, const char *wstr)
{
  return (!match(wstr, str));
#if 0  
  unsigned int i;
  
  if ((!str) || (!wstr)) return 1;
  
  while (1)
  {
    switch (*wstr)
    {
      case 0: return (!*str);
      case '*':
      {
        if (!wstr[0]) return 1;
        while (wstr[1] == '*') wstr++;
        for (i = 0; str[i]; i++)
        {
          if (wildcard_compare(str+i, wstr+1))
            return 1;
        }
        return 0;
      }      
      case '?':
      {
        if (!*str) return 0;
        str++;
        wstr++;
        break;
      }
      default:
      {
        if (!*str) return 0;
        while ((*str) && (*wstr) && (*wstr != '*') && (*wstr != '?'))
        {
          if (tolower(*str) != tolower(*wstr)) return 0;
          str++;
          wstr++;
        }
      }
    }
  }
  return 0;
#endif
}

/*
 *  match - stolen from ircu
 *
 */
int match(const char *mask, const char *string)
{
  const char *m = mask, *s = string;
  char ch;
  const char *bm, *bs;          /* Will be reg anyway on a decent CPU/compiler */
  
  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
    {         
      case '\\':      
        if (*m == '?' || *m == '*')
          ch = *m++;               
      default:      
        if (tolower(*s) != tolower(ch))
          return 1;
      case '?':    
        if (!*s++)
          return 1;
    };
  if (!ch)
    return *s;
      
  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;                       /* Next try rollback here */
  while ((ch = *m++))
    switch (ch)      
    {          
      case '?':
        if (!*s++)
          return 1;
      case '*':    
        bm = m;
        continue;               /* while */
      case '\\':                           
        if (*m == '?' || *m == '*')
          ch = *m++;
      default:      
        goto break_while;       /* C is structured ? */
    };             
break_while:
  if (!ch)
    return 0;                   /* mask ends with '*', we got it */
  ch = tolower(ch);
  while (tolower(*s++) != ch)
    if (!*s)
      return 1;
  bs = s;                       /* Next try start from here */
  
  /* Check the rest of the "chunk" */
  while ((ch = *m++))
  {
    switch (ch)
    {         
      case '*':
        goto got_star;
      case '\\':
        if (*m == '?' || *m == '*')
          ch = *m++;
      default:
        if (tolower(*s) != tolower(ch))
        {
          m = bm;
          s = bs;           
          goto got_star;
        };
      case '?':
        if (!*s++)
          return 1;
    };
  };
  if (*s)
  {
    m = bm;
    s = bs;
    goto got_star;
  };
  return 0;
}

const char *gtime(const time_t *clock)
{
  strcpy(gtime_str, asctime(gmtime(clock)));
  gtime_str[24] = ' ';
  strcat(gtime_str, "GMT");  
  return gtime_str; 
}

/**************************************************************************************************
 * string_chain_traverse
 **************************************************************************************************
 *   Loops through the entire string_chain, sending the strings in it as P10 commands
 *   to the server.
 **************************************************************************************************
 * Params:
 *   [IN] void *ptr  : The string_chain to run through
 **************************************************************************************************/
void string_chain_traverse(void *ptr)
{
  string_chain *chain = (string_chain *)ptr;
  while (chain)
  {
    com_send(irc, chain->str);
    chain = chain->next;
  }
}

/**************************************************************************************************
 * string_chain_free
 **************************************************************************************************
 *   Loops through the entire string_chain, freeing the resources.
 **************************************************************************************************
 * Params:
 *   [IN] void *ptr  : The string_chain to run through
 **************************************************************************************************/
void string_chain_free(void *ptr)
{
  string_chain *next, *chain = (string_chain *)ptr;
  while (chain)
  {
    next = chain->next;
    xfree(chain->str);
    xfree(chain);
    chain = next;
  }
}

/**************************************************************************************************
 * uppercase
 **************************************************************************************************
 *  Converts a string into uppercase
 **************************************************************************************************
 * Params:
 *   [IN] char *str  : The string you would like to convert to uppercase.
 * Return:
 *  [OUT] char*      : The converted string.
 **************************************************************************************************/
char *uppercase(char *str)
{
  char *save = str;

  while (*str) 
  {
    *str = toupper(*str);
    str++;
  }

  return save;
}

list_data irc_list_create(void)
{
  list_data list;
  list.list.generic = NULL;
  list.size = 0;
  list.allocated = 0;
  return list;
}

void irc_list_add(list_data *list, long position, void *elem)
{
  if (list->size == list->allocated)
  {
    list->allocated += LIST_ALLOC;
    list->list.generic = xrealloc(list->list.generic, list->allocated * sizeof(void*));
  }
  
  if (position < list->size)
    memmove(&list->list.generic[position + 1], &list->list.generic[position], (list->size - position) * sizeof(void*));
  
  list->size++;
  list->list.generic[position] = elem;
}

void irc_list_move(list_data *list, long from, long to)
{
  void *tmp;

  tmp = list->list.generic[from];

  if (to > from) memmove(&list->list.generic[from], &list->list.generic[from+1], (to - from) * sizeof(void*));
  else if (from > to) memmove(&list->list.generic[to+1], &list->list.generic[to], (from - to) * sizeof(void*));

  list->list.generic[to] = tmp;
}

void *irc_list_delete(list_data *list, long position)
{
  void *tmp;
  
  if (position >= list->size) return NULL;
  
  if (list->size < (list->allocated - LIST_ALLOC - 10))
  {
    list->allocated -= LIST_ALLOC;
    list->list.generic = xrealloc(list->list.generic, list->allocated * sizeof(void*));
  }
  tmp = list->list.generic[position];
  if (position != (list->size - 1))
    memmove(&list->list.generic[position], &list->list.generic[position + 1], (list->size - position - 1) * sizeof(void*));
  
  list->size--;
  
  return tmp;
}

void irc_list_clear(list_data *list)
{
  xfree(list->list.generic);
  list->list.generic = NULL;
  list->size = 0;
  list->allocated = 0;
}

/**************************************************************************************************
 * format_time
 **************************************************************************************************
 *  Converts a unix timestamp into a string (1 day 3 hours 7 minutes 18 seconds)
 **************************************************************************************************
 * Params:
 *   [IN] const time_t __tm  : The string you would like to convert to uppercase.
 * Return:
 *  [OUT] char*              : The converted timestamp.
 **************************************************************************************************/
const char *format_time(const time_t __tm)
{
  int years, weeks, days, hours, minutes, seconds;
  int tm = __tm;

  years = tm / 31449600;
  tm = tm % 31449600;

  weeks = tm / 604800;
  tm = tm % 604800;

  days = tm / 86400;
  tm = tm % 86400;

  hours = tm / 3600;
  tm = tm % 3600;

  minutes = tm / 60;
  seconds = tm % 60;

  format_time_str[0] = '\0';

  if (years)
    sprintf(format_time_str + strlen(format_time_str), "%dy ", years);
  if (weeks)
    sprintf(format_time_str + strlen(format_time_str), "%dw ", weeks);
  if (days)
    sprintf(format_time_str + strlen(format_time_str), "%dd ", days);
  if (hours)
    sprintf(format_time_str + strlen(format_time_str), "%dh ", hours);
  if (minutes)
    sprintf(format_time_str + strlen(format_time_str), "%dm ", minutes);
  if (seconds)
    sprintf(format_time_str + strlen(format_time_str), "%ds", seconds);

/*  sprintf(format_time_str, "%dy %dw %dd %dh %dm %ds", years, weeks, days, hours, minutes, seconds); */ 

  return format_time_str;
}

/**************************************************************************************************
 * set_is_ok
 **************************************************************************************************
 *   Converts the specified string to a numeric representation.
 *     0: if not recognized
 *     1: if off, no or 0
 *     2: if on, yes or 1
 **************************************************************************************************
 * Params:
 *   [IN] char *str: string to test if it's a valid set
 * Return;
 *  [OUT] int      : 0, 1 or 2, depending on the string
 **************************************************************************************************/
int set_is_ok(char *str)
{
  if (!str) return 0;
  
  /* uppercase the string */
  str = uppercase(str);

  /* compare to known words */
  if (!strcmp(str, "YES")) return 2;
  else if (!strcmp(str, "ON")) return 2;
  else if (!strcmp(str, "1")) return 2;
  else if (!strcmp(str, "OFF")) return 1;
  else if (!strcmp(str, "NO")) return 1;
  else if (!strcmp(str, "0")) return 1;

  /* not a known word */
  return 0;
}
