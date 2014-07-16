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

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char lastprog[2];
int standby = 0;

int displayinit(int rfd)
{
	return 0;
}

int displaytime(int rfd)
{
	struct timeval tv;
	struct tm *ptm;
	char message[] = { 0x20,0x60,0x00,0x00 };

	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);

	message[2] = ptm->tm_hour;             /* Hour */
	message[3] = ptm->tm_min;              /* Minutes */
	
	write(rfd, message, 4);

	return 0;
}

int displayupdate(int rfd)
{
	int ffd;
	char* filename="/tmp/dispdata";
	char buffer[4];
	
	ffd=open(filename,O_RDONLY);
	
	if (ffd < 0 )
		return 0;
	
	if ( 0 >= read(ffd,buffer,1)){
		close(ffd);
		return 0;
	}

	if ( buffer[0] == 'c')  /* Clock display */
	{
		displaytime(rfd);
	}
	else if (buffer[0] == 'p') /* Program display */
	{
		if ( 2 != read(ffd,buffer,2)){
			// Ignore
		}
		else
		{
			char message[] = { 0x20,0x40,0x00,0x00 };
		
			message[2] = buffer[0] & 0x0f;
			lastprog[0] = message[2];
			message[3] = buffer[1];
			lastprog[1] = message[3];
			if (!standby)	
				write(rfd, message, 4);
		}		
	}
	else if (buffer[0] == 'R') /* Record Start */
	{
		char message[] = { 0x20,0x80,0x00,0x00 };
		message[2] = lastprog[0];
		message[3] = lastprog[1];
		write(rfd, message, 4);
		message[1] = 0x40;
		write(rfd, message, 4);
	}
	else if (buffer[0] == 'r') /* Record Stop */
	{
		char message[] = { 0x20,0x82,0x00,0x00 };
		write(rfd, message, 4);
		
		if (standby) {	
			usleep(100000);
			message[1] = 0x65;
			write(rfd, message, 4);
		}
	}
	else if (buffer[0] == 'S') /* Standby Start */
	{
		char message[] = { 0x20,0x65,0x00,0x00 };
		write(rfd, message, 4);
		standby = 1;
		
		usleep(100000);
		displaytime(rfd);
	}
	else if (buffer[0] == 's') /* Standby Stop */
	{
		standby = 0;
                char message[] = { 0x20,0x40,0x00,0x00 };
                message[2] = lastprog[0];
                message[3] = lastprog[1];
                write(rfd, message, 4);
	}
	close(ffd);

	return 0;
}

