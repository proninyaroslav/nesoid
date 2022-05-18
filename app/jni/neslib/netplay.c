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

#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "svga.h"
#include "netplay.h"

int netplay=0;         /*      1 if we are a server */
                        /*      2 if we need a host  */
static uint8 netjoy[4];	// controller stuff.

static void NetError(void)
{
 FCEU_DispMessage("Network error/connection lost!");
// FCEUD_PrintError("Network error/connection lost!");
 netplay=0;
 FCEUD_NetworkClose();
}

void KillNetplay(void)
{
 if(netplay)
 {
  FCEUD_NetworkClose();
  netplay=0;
 }
}

int InitNetplay(void)
{
         if(!FCEUD_NetworkConnect())
          {NetError();return 0;}
         netplay=FSettings.NetworkPlay;
	 memset(netjoy,0,sizeof(netjoy));
         return 1;
}

void NetplayUpdate(uint16 *joyp1, uint16 *joyp2)
{
 uint8 buf[5];
 if(netplay==1)         // We're the server
 {
  int t;

  loopo:

  t=FCEUD_NetworkRecvData(&netjoy[2],1,0);
  if(!t) {NetError();return;}   
  if(t!=-1) goto loopo;

  netjoy[0]=*joyp1;
  memcpy(buf,netjoy,4);
  buf[4]=CommandQueue;

  if(!FCEUD_NetworkSendData(buf,5)) {NetError();return;}
  if(CommandQueue)
  {
   DoCommand(CommandQueue);
   CommandQueue=0;
  }
 }
 else if(netplay==2)    // We're connected to a host (we're second player)
 {
  uint8 ja=(*joyp1)|(*joyp1>>8)|(*joyp2)|(*joyp2>>8);
  if(!FCEUD_NetworkSendData(&ja,1)) {NetError();return;}
  if(!FCEUD_NetworkRecvData(buf,5,1)) {NetError();return;}

  memcpy(netjoy,buf,4);
  if(buf[4]) DoCommand(buf[4]);
 }
 *joyp1=netjoy[0]|(netjoy[1]<<8);
 *joyp2=netjoy[2]|(netjoy[3]<<8);

}
#endif
