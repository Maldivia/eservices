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
/* $Id: setup.h,v 1.11 2004/08/06 17:59:47 mr Exp $ */

#ifndef INC_SETUP_H
#define INC_SETUP_H

#include "defines.h"


/* Defines the NETWORK_NAME */
#define NETWORK_NAME "VikingIRC"


/* Defines the config file which is going to be used. */
#define CONFIG_FILE "services.conf"


/* Defines used in the auto ignoring feature, which will ignore
   noisy/spammy users.

   IGNORE_LINES  is the number of lines that services will accept from
                 a user in IGNORE_TIME
   IGNORE_LENGTH is the time that the user will be ignored for.

   All times are in seconds. */ 
#define IGNORE_LINES    5
#define IGNORE_TIME    15
#define IGNORE_LENGTH 120


/* uncomment the below statement, to disable on message at renick and on
   connect informing the user how to register his/her nick */
/* #define NO_NICKSERV_REGISTER_NICK_ANNOUNCE */


/* uncomment the below statement, to disable on connect welcome message */
/* #define NO_NICKSERV_WELCOME */


/* uncomment the below statement, if services should only be useable
   by opers (umode +o) */
/* #define SERVICES_OPER_ONLY */


/* defines the number of seconds waiting for a PONG */
#define PING_WAIT 45


/* Defines whether or not the sql interface in operserv
   should be activated. */
#define SQL_INTERFACE_ACTIVATED 1




/* These defines are message tokens.
   If you change these the service might not work properly!  */
#define MODE_PRIVMSG "P"
#define MODE_NOTICE  "O"

/* Default buffer size */
#define BUFFER_SIZE   712
#define MAX_NICK_LEN   35
#define LIST_ALLOC   1024

/* DEBUG macros 
   You should under no cirsumstances change these!
 */
#ifdef NDEBUG
#define debug_out(x...)
#else
#define debug_out(x...) printf(x)
#endif

#endif /* INC_SETUP_H */
