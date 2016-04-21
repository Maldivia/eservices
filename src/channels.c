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
/* $Id: channels.c,v 1.8 2003/10/20 11:48:11 cure Exp $ */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "setup.h"
#include "dbase.h"
#include "server.h"
#include "misc_func.h"
#include "config.h"
#include "chanserv.h"

extern sock_info *irc;
static list_data channels;

void channels_init(void)
{
  channels = irc_list_create();
}

/********************************************************************
  CHANNELS_SEARCH
     Searches the channel-array for the given channel
       
  Parameters and return:
    [in]  char *name   = the channel to find
    [out] long  return
                  >= 0 = index in the channel array pointing at the
                         successfully found channel 
                   < 0 = Error - channel not found
                        (Internal use: (return+1)*-1 = index to nearest )
   
********************************************************************/
long channels_search(const char *name)
{
  return channels_internal_search(0, channels.size - 1, name);
}

/********************************************************************
  CHANNELS_INTERNAL_SEARCH
     Used internally by channels_search to do a bintree-search
     in the channel-array
        
  Parameters and return:
    [in]  long  low    = \ Only the segment between low and high in
    [in]  long  high   = / the array are searched
    [in]  char *name   = name of channel to find
    [out] long  return
                  >= 0 = index in the channel array pointing at the
                         successfully found channel 
                   < 0 = Error - channel not found
                        (Internal use: (return+1)*-1 = index to nearest )
   
********************************************************************/
long channels_internal_search(long low, long high, const char *name)
{
  int res;
  long mid = high - ((high - low) / 2);
  if (low > high) return -1-low;
  res = strcasecmp(name, channels.list.channels[mid]->name);
  if (res < 0) return channels_internal_search(low, mid-1, name);
  else if (res > 0) return channels_internal_search(mid+1, high, name);
  else return mid;
}

/********************************************************************
  CHANNELS_ADD
    Adds a channem to the channel array by allocating a pointer to
    which the information from the data parameter are COPIED,
    pointers are of course allocated and used with strcpy/memcpy.
    
    Make sure ALL elements of data are valid!    
    
    OBS: If a channel with the same name already exists, it WILL
         be overwritten !!

    channels_add uses channels_search to find the place in the array
    where the new channel should be placed, to ensure the array is
    always alpha-sorted.
        
  Parameters and return:
    [in]  dbase_channels *data = struct containing the data to add
    [out] long  return      = index in the channel array to the
                              newly added channel
   
********************************************************************/
long channels_add(const char *name, long createtime)
{ 
  long nr;
  if ((nr = channels_search(name)) < 0)
  {
    nr += 1;
    nr *= -1;
    irc_list_add(&channels, nr, xmalloc(sizeof(dbase_channels)));
  }
  else
  {
    return nr;
  }

  channels.list.channels[nr]->name = (char *)xmalloc(sizeof(char)*(strlen(name)+1));
  strcpy(channels.list.channels[nr]->name, name);

  channels.list.channels[nr]->topic = NULL;
  channels.list.channels[nr]->modes = 0;
  channels.list.channels[nr]->bancount = 0;
  channels.list.channels[nr]->bans = NULL;
  channels.list.channels[nr]->key = NULL;

  channels.list.channels[nr]->users = (dbase_channels_nicks **)xcalloc(0, sizeof(dbase_channels_nicks*));
  
  channels.list.channels[nr]->usercount = 0;
  channels.list.channels[nr]->limit = 0;
  channels.list.channels[nr]->createtime = createtime;

  channels.list.channels[nr]->chanserv = NULL;

  return nr;
}

/********************************************************************
  CHANNELS_REMOVE
    Removes a channel from the channel array and free()'s all
    recources associated with the channel.

  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with
                          name=<name> is found and used
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - channel removed
   
********************************************************************/
long channels_remove(long index, const char *name)
{
  int i;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;
  if (channels.list.channels[index])
  {
    xfree(channels.list.channels[index]->name);
    xfree(channels.list.channels[index]->topic);
    xfree(channels.list.channels[index]->users);
    xfree(channels.list.channels[index]->key);
      for (i=0;i<channels.list.channels[index]->bancount;i++)
        xfree(channels.list.channels[index]->bans[i]);
      xfree(channels.list.channels[index]->bans);
    xfree(channels.list.channels[index]);
  }
  
  irc_list_delete(&channels, index);
  return index;
}

/********************************************************************
  CHANNELS_SETMODE
    Changes the channel's modes
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  char *modes   = the channel'snew modes
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - channel modes set
   
********************************************************************/
long channels_setmode(long index, const char *name, char *modes)
{
  unsigned int i, check = 1;
  if (!modes) return 0;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  for (i = 0; i < strlen(modes); i++)
  {
    if (modes[i] == '+') check = 1;
    else if (modes[i] == '-') check = 0;
    else if ((modes[i] >= 'a') && (modes[i] <= 'z'))
    {
      if (check) bitadd(channels.list.channels[index]->modes, modes[i]-'a');
      else bitdel(channels.list.channels[index]->modes, modes[i]-'a');
    }   
  }

  return index;
}

/********************************************************************
  CHANNELS_SETTOPIC
    Changes the channel's topic
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  long  topic   = The topic to set
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - Topic changed
   
********************************************************************/
long channels_settopic(long index, const char *name, char *topic)
{
  if (!topic) return 0;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  channels.list.channels[index]->topic = (char*)xrealloc(channels.list.channels[index]->topic, sizeof(char)*(strlen(topic)+1));
  strcpy(channels.list.channels[index]->topic, topic);

  return index;
}


/********************************************************************
  CHANNELS_ADDLIMIT
    Changes the channel's limit variable
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  long  limit   = the new limit
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - Limit set
   
********************************************************************/
long channels_addlimit(long index, const char *name, long limit)
{
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;
  channels.list.channels[index]->limit = limit;
  channels_setmode(index, NULL, "+l");  
  return index;
}

/********************************************************************
  CHANNELS_REMLIMIT
    removes the channel's limit variable (set to 0 (zero))
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - Limit removed
   
********************************************************************/
long channels_remlimit(long index, const char *name)
{
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;
  channels.list.channels[index]->limit = 0;
  channels_setmode(index, NULL, "-l");
  return index;
}


/********************************************************************
  CHANNELS_ADDKEY
    Changes channel's key variable (channel password)
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  char *key     = The new key
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - key set
   
********************************************************************/
long channels_addkey(long index, const char *name, char *key)
{
  if (!key) return 0;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  channels.list.channels[index]->key = (char *)xrealloc(channels.list.channels[index]->key, sizeof(char)*(strlen(key)+1));
  strcpy(channels.list.channels[index]->key, key);
    channels_setmode(index, NULL, "+k");

  return index;
}

/********************************************************************
  CHANNELS_REMKEY
    Removes channels key variable (set to NULL)
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - key removes
   
********************************************************************/
long channels_remkey(long index, const char *name, const char *key)
{
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  if (!channels.list.channels[index]->key) return -1;
  if (strcmp(channels.list.channels[index]->key, key)) return -1;
  xfree(channels.list.channels[index]->key);
  
  channels.list.channels[index]->key = NULL;
  channels_setmode(index, NULL, "-k");
  return index;
}

/********************************************************************
  CHANNELS_ADDBAN
    Adds a ban to the channel's ban list
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  char *ban     = the nick!user@host to add do the list
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - ban added
   
********************************************************************/
long channels_addban(long index, const char *name, char *bans)
{
  char *tmp;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  if (bans == NULL) return -1*index;
  if (strlen(bans) < 1) return -1*index;

  do
  {    
    if ((tmp = strchr(bans, ' ')))
    {
      while (tmp[0] == ' ')
      {
        tmp[0] = '\0';
        tmp++;
      }
    }
    
    if (!channels_check_ban_covered(index, name, bans))
    {
      channels_remove_covered_ban(index, name, bans);
      channels.list.channels[index]->bans = (char **)xrealloc(channels.list.channels[index]->bans, sizeof(char *)*(++channels.list.channels[index]->bancount));
      channels.list.channels[index]->bans[channels.list.channels[index]->bancount - 1] = (char*)xmalloc(sizeof(char)*(strlen(bans)+1));
      strcpy(channels.list.channels[index]->bans[channels.list.channels[index]->bancount - 1], bans);
    }
    bans = tmp;
  } while (bans);
  return index;
}

/********************************************************************
  CHANNELS_REMBAN
    Removes a ban to the channel's ban list
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  char *ban     = the nick!user@host to remove from the list
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - ban removes
   
********************************************************************/
long channels_remban(long index, const char *name, char *bans)
{
  char *tmp;
  int i;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  if (bans == NULL) return -1*index;
  if (strlen(bans) < 1) return -1*index;

  while (bans)
  {
    if ((tmp = strchr(bans, ' ')))
    {
      while (tmp[0] == ' ') *tmp++ = '\0';
    }
    if (!chanserv_dbase_check_enforced_ban(channels.list.channels[index]->name, bans))
    {
      for (i=0; i<channels.list.channels[index]->bancount; i++)
      {
        if (strcasecmp(bans, channels.list.channels[index]->bans[i]) == 0)
        {
          xfree(channels.list.channels[index]->bans[i]);
          channels.list.channels[index]->bans[i] = channels.list.channels[index]->bans[--channels.list.channels[index]->bancount];
          channels.list.channels[index]->bans = (char **)xrealloc(channels.list.channels[index]->bans, sizeof(char*)*(channels.list.channels[index]->bancount));
          break;
        }
      }
    }
    bans = tmp;
  }
  return index;
}

int channels_check_ban_covered(long index, const char *name, const char *ban)
{
  int i;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return 0;

  if (ban == NULL) return 0;
  if (strlen(ban) < 1) return 0;
    
  for (i=0; i < channels.list.channels[index]->bancount; i++)
    if (wildcard_compare(ban, channels.list.channels[index]->bans[i])) return 1;
      
  return 0;
}

long channels_remove_covered_ban(long index, const char *name, const char *ban)
{
  int i;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;

  if (ban == NULL) return -1*index;
  if (strlen(ban) < 1) return -1*index;
    
  for (i=0; i < channels.list.channels[index]->bancount; i++)
  {
    if (wildcard_compare(channels.list.channels[index]->bans[i], ban))
    {
      xfree(channels.list.channels[index]->bans[i]);
      channels.list.channels[index]->bans[i] = channels.list.channels[index]->bans[--channels.list.channels[index]->bancount];
      channels.list.channels[index]->bans = (char **)xrealloc(channels.list.channels[index]->bans, sizeof(char*)*(channels.list.channels[index]->bancount));
      i--;
    }
  }
  return index;
}

/********************************************************************
  CHANNELS_USERJOIN
    Adds a user to the channel's user-list
        
  Parameters and return:
    [in]  long  index            = channel array index (or <0)
    [in]  char *name             = if index<0 index for channel with 
                                   name=<name> is found and used
    [in]  dbase_channels_users user = user to add
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - user added
   
********************************************************************/
long channels_userjoin(long index, const char *name, const char *mode, const char *numeric)
{
  long nr;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0)
  {
    if ((index = channels_add(name, time(0))) < 0) return index;
  }

  if ((nr = channels_user_search(channels.list.channels[index]->users, 0, channels.list.channels[index]->usercount-1, numeric)) < 0)
  {
    nr += 1;
    nr *= -1;
    channels.list.channels[index]->users = (dbase_channels_nicks **) xrealloc(channels.list.channels[index]->users, (channels.list.channels[index]->usercount+1) * sizeof(dbase_channels_nicks*));
    if (nr < (channels.list.channels[index]->usercount++))
      memmove(&channels.list.channels[index]->users[nr+1], &channels.list.channels[index]->users[nr], (channels.list.channels[index]->usercount - nr - 1) * sizeof(dbase_channels_nicks*));
  }
  else
    return -1*nr;
    
  channels.list.channels[index]->users[nr] = xmalloc(sizeof(dbase_channels_nicks));
  channels.list.channels[index]->users[nr]->channel = channels.list.channels[index];
  channels.list.channels[index]->users[nr]->mode = 0;
  nicks_join_channel(numeric, channels.list.channels[index]->users[nr]);
  channels_usermode(index, name, mode, numeric);
  if (channels.list.channels[index]->users[nr]->nick->nickserv)
  {
    chanserv_dbase_access *ac = chanserv_dbase_has_access(channels.list.channels[index]->users[nr]->nick->nickserv->nick, chanserv_dbase_find_chan(channels.list.channels[index]->name));
    if (ac)
    {
      /* Check if the user has autoop - and the channel is not disabled */
      if ((ac->autoop) && (channels.list.channels[index]->chanserv))
      {
        channels_usermode(index, name, "+o", numeric);
        com_send(irc, "%s M %s +o %s\n", conf->cs->numeric, name, numeric);
        chanserv_dbase_update_lastlogin(channels.list.channels[index]->chanserv);
      }
    }
  }
  return nr;    
}

/********************************************************************
  CHANNELS_USERPART
    Removes a user from the channel's user-list
    
    OSB: If user-count <= 0 - the channel is automatically removed
         by calling channels_remove
        
  Parameters and return:
    [in]  long  index            = channel array index (or <0)
    [in]  char *name             = if index<0 index for channel with 
                                   name=<name> is found and used
    [in]  dbase_channels_users user = user to remove
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - User removed from list
   
********************************************************************/
long channels_userpart(long index, const char *name,  const char *numeric)
{
  long nr;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;
  if ((nr = channels_user_search(channels.list.channels[index]->users, 0, channels.list.channels[index]->usercount-1, numeric)) < 0)
    return nr;
    
  nicks_part_channel(numeric, channels.list.channels[index]->users[nr]);
  xfree(channels.list.channels[index]->users[nr]);

  memmove(&channels.list.channels[index]->users[nr], &channels.list.channels[index]->users[nr+1], (channels.list.channels[index]->usercount - nr - 1) * sizeof(dbase_channels_nicks*));
  channels.list.channels[index]->users = (dbase_channels_nicks **) xrealloc(channels.list.channels[index]->users, --channels.list.channels[index]->usercount * sizeof(dbase_channels_nicks*));

  if (channels.list.channels[index]->usercount <= 0)
    return channels_remove(index, name);

  return nr;
}

/********************************************************************
  CHANNELS_USERMODE
    Changes the usermode for the specified user (numeric)
        
  Parameters and return:
    [in]  long  index   = channel array index (or <0)
    [in]  char *name    = if index<0 index for channel with 
                          name=<name> is found and used
    [in]  char *mode    = the mode(s) to set
    {in]  char *numeric = Numeric of user to set mode(s) on
    [out] long  return 
                   < 0  = Error
                  >= 0  = OK - usermode changed
   
********************************************************************/
long channels_usermode(long index, const char *name, const char *mode, const char *numeric)
{  
  long nr;
  int sign = 1;
  const char *m;
  if (!mode)  return 1;
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return index;
  nr = channels_user_search(channels.list.channels[index]->users, 0, channels.list.channels[index]->usercount-1, numeric);
  if (nr < 0) return nr;
  m = mode;
  while (m[0] != '\0')
  {
    switch (m[0])
    {
      case '+' : sign =  1; break;
      case '-' : sign = -1; break;
      case 'o' : channels.list.channels[index]->users[nr]->mode += sign*2; break;
      case 'v' : channels.list.channels[index]->users[nr]->mode += sign*1; break;
    }
    m++;
  }
  return nr;
}

/********************************************************************
  CHANNELS_USER_SEARCH
     Searches the channel's user-list for the given user
        
  Parameters and return:
    [in] dbase_channels_users *arr = pointer to the channel's user-list
    [in] long  low              = \ Only the segment between low and
    [in] long  high             = / high in the array are searched
    [in] char *numeric          = numeric to look for
   [out] long  return
                           < 0  = User not found
                          >= 0  = User found, index to user returned
   
********************************************************************/
long channels_user_search(dbase_channels_nicks **arr, long low, long high,  const char *numeric)
{
  int res;
  long mid = high - ((high - low) / 2);
  if (low > high) return -1-low;
  res = strcmp(numeric, arr[mid]->nick->numeric);
  if (res < 0) return channels_user_search(arr, low, mid-1, numeric);
  else if (res > 0) return channels_user_search(arr, mid+1, high, numeric);
  else return mid;
}

/********************************************************************
  CHANNELS_GETINFO
    Retrives a pointer to the channel's struct in memory
        
  Parameters and return:
    [in]  long  index          = channel array index (or <0)
    [in]  char *name           = if index<0 index for channel with 
                                 name=<name> is found and used
    [out] dbase_channels *return
                          NULL = Error
                          else = Pointer to the channel's struct
   
********************************************************************/
dbase_channels *channels_getinfo(long index, const char *name)
{
  if ((index < 0) || (index >= channels.size)) index = channels_search(name);
  if (index < 0) return NULL;
  return channels.list.channels[index];
}

/********************************************************************
  CHANNELS_GETCOUNT
     Returns the total number of channels in the channel array
        
  Parameters and return:
    [out] long return = the channel count
   
********************************************************************/
long channels_getcount(void)
{
  return channels.size;
}

void channels_cleanup(void)
{
  debug_out(" | |==> Cleaning Channels database...\n");
  while (channels.size > 0)
    channels_remove(channels.size -1, NULL);
}


long channels_user_find(const char *chan, const char *numeric)
{
  int nr;
  dbase_channels *c;
  c = channels_getinfo(-1, chan);
  if (!c) return -1;

  nr = channels_user_search(c->users, 0, c->usercount - 1, numeric);
  if (nr < 0) return nr;
  return c->users[nr]->mode;
}
