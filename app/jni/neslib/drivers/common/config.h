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

typedef struct {
        char *name;
        void *ptr;
        int len;
} CFGSTRUCT;

int SaveFCEUConfig(char *filename, CFGSTRUCT *cfgst);
int LoadFCEUConfig(char *filename, CFGSTRUCT *cfgst);

/* Macros for building CFGSTRUCT structures. */

/* CFGSTRUCT structures must always end with ENDCFGSTRUCT */
#define ENDCFGSTRUCT	{ 0,0,0 }

/* When this macro is used, the config loading/saving code will parse
   the new config structure until the end of it is detected, then it
   will continue parsing the original config structure.
*/
#define ADDCFGSTRUCT(x) { 0,&x,0 }

/* Oops.  The NAC* macros shouldn't have the # in front of the w, but
   fixing this would break configuration files of previous versions and it
   isn't really hurting much.
*/

/* Single piece of data(integer). */
#define AC(x)   { #x,&x,sizeof(x)}
#define NAC(w,x) { #w,&x,sizeof(x)}

/* Array. */
#define ACA(x)   {#x,x,sizeof(x)}
#define NACA(w,x) {#w,x,sizeof(x)}

/* String(pointer) with automatic memory allocation. */
#define ACS(x)  {#x,&x,0}
#define NACS(w,x)  {#w,&x,0}

