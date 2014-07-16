/*
  smtlircd: a lirc daemon for the Samsung SMT 7020S Set-Top-Box
Copyright (C) 2006 D.Herrendoerfer

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

int displayinit(int rfd);
int displaytime(int rfd);

char *getlircmessage(int key);

void nolinger(int sock);
int setupSocket();

/* constants
 * */
#define BAUDRATE        B9600
#define SERIAL_PORT     "/dev/ttyS1"
#define MAX_RECV_LEN    32

