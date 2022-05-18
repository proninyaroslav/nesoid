/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 Bero
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

void FASTAPASS(2) VRAM_BANK1(uint32 A, uint8 V)
{
 V&=7;
 PPUCHRRAM|=(1<<(A>>10));
 CHRBankList[(A)>>10]=V;
 VPage[(A)>>10]=&CHRRAM[V<<10]-(A);
}

void FASTAPASS(2) VRAM_BANK4(uint32 A, uint32 V)
{
 V&=1;
 PPUCHRRAM|=(0xF<<(A>>10));
 CHRBankList[(A)>>10]=(V<<2);
 CHRBankList[((A)>>10)+1]=(V<<2)+1;
 CHRBankList[((A)>>10)+2]=(V<<2)+2;
 CHRBankList[((A)>>10)+3]=(V<<2)+3;
 VPage[(A)>>10]=&CHRRAM[V<<10]-(A);
}

void FASTAPASS(2) VROM_BANK1(uint32 A,uint32 V)
{
 setchr1(A,V);
 CHRBankList[(A)>>10]=V;
}

void FASTAPASS(2) VROM_BANK2(uint32 A,uint32 V)
{
 setchr2(A,V);
 CHRBankList[(A)>>10]=(V<<1);
 CHRBankList[((A)>>10)+1]=(V<<1)+1;
}

void FASTAPASS(2) VROM_BANK4(uint32 A, uint32 V)
{
 setchr4(A,V);
 CHRBankList[(A)>>10]=(V<<2);
 CHRBankList[((A)>>10)+1]=(V<<2)+1;
 CHRBankList[((A)>>10)+2]=(V<<2)+2;
 CHRBankList[((A)>>10)+3]=(V<<2)+3;
}

void FASTAPASS(1) VROM_BANK8(uint32 V)
{
 setchr8(V);
 CHRBankList[0]=(V<<3);
 CHRBankList[1]=(V<<3)+1;
 CHRBankList[2]=(V<<3)+2;
 CHRBankList[3]=(V<<3)+3;
 CHRBankList[4]=(V<<3)+4;
 CHRBankList[5]=(V<<3)+5;
 CHRBankList[6]=(V<<3)+6;
 CHRBankList[7]=(V<<3)+7;
}

void FASTAPASS(2) ROM_BANK8(uint32 A, uint32 V)
{
 setprg8(A,V);
 if(A>=0x8000)
  PRGBankList[((A-0x8000)>>13)]=V;
}

void FASTAPASS(2) ROM_BANK16(uint32 A, uint32 V)
{
 setprg16(A,V);
 if(A>=0x8000)
 {
  PRGBankList[((A-0x8000)>>13)]=V<<1;
  PRGBankList[((A-0x8000)>>13)+1]=(V<<1)+1;
 }
}

void FASTAPASS(2) ROM_BANK32(uint32 V)
{
 setprg32(0x8000,V);
 PRGBankList[0]=V<<2;
 PRGBankList[1]=(V<<2)+1;
 PRGBankList[2]=(V<<2)+2;
 PRGBankList[3]=(V<<2)+3; 
}

