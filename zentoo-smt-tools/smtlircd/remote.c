/*
 smtlircd: a lirc daemon for the Samsung SMT 7020S Set-Top-Box
 Copyright (C) 2007 D.Herrendoerfer

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>

#include "smtlircd.h"

int lastkey;
char messagebuf[128];

char *keys[256] = {
        "POWER", "TONLINE", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "*", "EXIT", "", "",
        "MEDIA", "VOLUP", "VOLDOWN", "MUTE", "CHUP", "CHDOWN", "EPG", "REW", "STOP", "PLAY", "FWD", "RED", "GREEN", "YELLOW", "BLUE", "UP",
        "DOWN", "LEFT", "RIGHT", "OK", "PREV", "", "", "NEXT", "", "AUDIO", "INFO", "FAV", "MENU", "TV", "REC", "ASPECT",
        "TEXT", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "OK", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "POWERFP", "", "", "", "", "DOWN", "UP", "LEFT", "RIGHT", "", "MENU", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

char *getlircmessage(int key)
{
	if (key < 0x100){
		//if ((lastkey == 0x19 || lastkey == 0x17 || lastkey == 0x1a) && key == 0x19 ) {
		if (lastkey != 0xff && key == 0x19 ) {
			sprintf(messagebuf,"00000000000000ff 00 PAUSE SMT\n");
			lastkey = 0xff;
		} else {
			sprintf(messagebuf,"000000000000%04x 00 %s SMT\n",key,keys[key]);
			lastkey = key;
		}

		return messagebuf;
	}
			
	return NULL;
}

