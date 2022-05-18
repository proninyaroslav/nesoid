/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#ifndef socklen_t
#define socklen_t int
#endif
#ifdef NETWORK
static int Socket=-1;
#endif
#include "main.h"
#include "unix-netplay.h"

char *netplayhost=0;
int Port=0xFCE;
int netplay=0;

int FCEUD_NetworkConnect(void)
{
#ifdef NETWORK
 struct sockaddr_in sockn;
 int TSocket;

 memset(&sockn,0,sizeof(sockn));
 sockn.sin_family=AF_INET;
 sockn.sin_port=htons(Port);

 if((TSocket=socket(AF_INET, SOCK_STREAM, 0))<0)
 {
  puts("Error creating socket.");
  return(0);
 }

 if(netplay==1)		/* Be a server. */
 {
  sockn.sin_addr.s_addr=INADDR_ANY;
  if(bind(TSocket, (struct sockaddr *)&sockn, sizeof(sockn))<0)
  {
   close(TSocket);
   puts("Error binding to socket."); 
   return(0);
  }
  if(listen(TSocket, 1)<0)
  {
   puts("Error listening on socket.");
   close(TSocket);
   return(0);
  }
  {
   socklen_t len=sizeof(sockn);
     
   printf("Accepting connection on port %d...\n",Port);
   if((Socket=accept(TSocket,(struct sockaddr *)&sockn,&len))<0 )
   {
    puts("Error accepting a connection.");
    close(TSocket);
    return(0);
   }
   close(TSocket);
  }

 }
 else /* Connect as a client if not a server. */
 {
  struct hostent *Host;

  if((sockn.sin_addr.s_addr=inet_addr(netplayhost))==INADDR_NONE)
  {
   if(!(Host=gethostbyname(netplayhost)))
   {
    puts("Error getting network host entry.");
    return(0);
   }
   memcpy(&sockn.sin_addr,Host->h_addr,Host->h_length);
  }  
  printf("Attempting to connect to %s...\n",netplayhost);
  if( connect(TSocket, (struct sockaddr *)&sockn, sizeof(sockn)) <0 )
  {
   puts("Error connecting to remote host.");
   close(TSocket);
   return(0);
  }
  Socket=TSocket;
 }
#endif
 return(1);  
}

/* 0 on failure, 1 on success, -1 if it would block and blocking is not
   specified.
*/

int FCEUD_NetworkRecvData(uint8 *data, uint32 len, int block)
{
#ifdef NETWORK
  if(block)
  {
   int t;
   uint8 temp[32];
   t=recv(Socket,temp,32,MSG_PEEK|MSG_DONTWAIT);
   if(t==-1)
   {
    if(errno!=EAGAIN) return(0);
   }
   else if(t==32)
    NoWaiting|=2;
   else
    NoWaiting&=~2;
   return(recv(Socket,data,len,0)==len);
  }
  else
  {
   int t=recv(Socket,data,len,MSG_DONTWAIT);
   if(t==-1)
   {
    if(errno==EAGAIN)   // Would block
     return(-1);
    return(0);
   }
   return(1);
  }
#else  
  return 1;
#endif
}

/* 0 on failure, 1 on success.  This function should always block. */

int FCEUD_NetworkSendData(uint8 *Value, uint32 len)
{
#ifdef NETWORK
 return(send(Socket,Value,len,0)==len);
#else
 return 0;
#endif
}

void FCEUD_NetworkClose(void)
{
#ifdef NETWORK
 if(Socket>0)
  close(Socket);
 Socket=-1;
#endif
}

