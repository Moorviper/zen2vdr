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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "smtlircd.h"

int sockfd,lastvalue,lastcount;
char *lircdfile = "/dev/lircd";
char *progname = "lirc.c";
mode_t permission=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
int clients[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int num_clients = 0;
int  ipid = 0;

struct termios oldPortSettings;
struct termios newPortSettings;

int frameSize;		/* Frame size */
int progRun;		/* 0: Stop loop 1: Run loop */
int frameFound;		/* 0 < means frame begin found */


int msgindex;
unsigned char buffer[MAX_RECV_LEN];


inline int max(int a,int b)
{
        return(a>b ? a:b);
}

void nolinger(int sock)
{
	static struct linger  linger = {0, 0};
	int lsize  = sizeof(struct linger);
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
}

int setupSocket()
{
	int ret;
	int new = 0;
	struct sockaddr_un serv_addr;
	struct stat s;

	/* create socket */
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "%s: could not create socket\n", progname);
		perror(progname);
		goto start_server_failed;
	}

	/* 
	 *         get owner, permissions, etc.
	 *         so new socket can be the same since we
	 *         have to delete the old socket.  
	 **/
	ret = stat(lircdfile, &s);
	if (ret == -1 && errno != ENOENT) {
		fprintf(stderr,
			"%s: could not get file information for %s\n",
			progname, lircdfile);
		perror(progname);
		goto start_server_failed;
	}
	if (ret != -1) {
		ret = unlink(lircdfile);
		if (ret == -1) {
			fprintf(stderr, "%s: could not delete %s\n",
				progname, lircdfile);
			perror(NULL);
			goto start_server_failed;
		}
	}

	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, lircdfile);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
	    == -1) {
		fprintf(stderr, "%s: could not assign address to socket\n",
			progname);
		perror(progname);
		goto start_server_failed;
	}

	if (new ?
	    chmod(lircdfile, permission) :
	    (chmod(lircdfile, s.st_mode) == -1 ||
	     chown(lircdfile, s.st_uid, s.st_gid) == -1)
	    ) {
		fprintf(stderr, "%s: could not set file permissions\n",
			progname);
		perror(progname);
		goto start_server_failed;
	}

	listen(sockfd, 3);
	nolinger(sockfd);

	return 0;

start_server_failed:
	return -1;
}

void ioreset (int fd)
{
	/* Set old port settings */
	tcsetattr(fd, TCSANOW, &oldPortSettings);

	/* Close serial port */
	close(fd);
}

int openserdev(void)
{
	int fd;
	
	/* Open serial port */
	fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd < 0) {
		perror("Unable to open serial port.");
		return -1;
	}

	fcntl(fd, F_SETFL, 0);

	/* Save current port settings */
	tcgetattr(fd, &oldPortSettings);

	/* Set new port settings (8n1 without flow control) */
	bzero(&newPortSettings, sizeof(newPortSettings));
	newPortSettings.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD | CSTOPB;
	newPortSettings.c_iflag = IGNPAR | IXON | IXOFF;
	newPortSettings.c_oflag = 0;	/* Raw output */
	newPortSettings.c_lflag = 0;	/* Raw input */
	newPortSettings.c_cc[VTIME] = 10;	/* Time to wait for data */
	newPortSettings.c_cc[VMIN] = 0;	/* No blocking read */
	tcsetattr(fd, TCSANOW, &newPortSettings);

	/* Flush serial port */
	tcflush(fd, TCIFLUSH);

	return fd;
}

int sendlircmsg(int* clients, char* message)
{
	int i;
	
	for (i=0 ; i<16 ; i++)
	{
		if ( clients[i] != -1 )
		{
			int done,todo,len;
			char *buf;

			len=strlen(message);
			buf=message;
			todo=len;

			while(todo)
			{
					done=write(clients[i],buf,todo);
					if(done<=0)
					{	
						clients[i]=-1;
						num_clients--;
						break;
					}
					buf+=done;
					todo-=done;
			}
		}
	}
	
	return 0;
}

int getmsg (int fd)
{
	ssize_t readSize = 0;		/* Number of read bytes */	
	int key = -1;
	
	if (msgindex == MAX_RECV_LEN)
	{
		perror("Frame too long.\n");
		msgindex = 0;
		frameFound = 0;
	}
	
	/* Read */
	readSize = read(fd, &buffer[msgindex], 1);

	if (0 > readSize) {
		perror("Couldn't read from serial port.");
		return -1;
	}
#ifdef DEBUG
		printf("read:%i -> 0x%02x\n",msgindex,buffer[msgindex]);
#endif
		/* Begin of a frame found? */
	if (0 == frameFound) {
		if (0x89 == buffer[msgindex] || 0x97 == buffer[msgindex]) {
			/* Frame begin found */
			frameFound = 1;
		}
	} else
		/* Frame size? */
	if (1 == frameFound) {
		if (0x89 == buffer[msgindex] || 0xa0 == buffer[msgindex]) {
			/* Frame begin found */
			frameFound = 2;
			frameSize = 3;
		}
	} else
		/* Frame end? */
	if (msgindex == frameSize) {
		int i;
		int mod = 0;
		
		for (i = 0; i <= frameSize ; i++) {
			mod ^= buffer[i];
			
//			printf("mod 0x%04x\n",mod);
		}
		mod = mod & 0xFF;
		if (mod == 0xff)
		{	
//			printf("got msg w. checksum\n");
			key=buffer[2];
		}
		else
		{
//			printf("got msg\n");
			key=0x40 + buffer[3];
		}

		/* Start again */
		msgindex = 0;
		frameFound = 0;
	}

	/* Count only if we know the begin of a frame */
	if (0 < frameFound) {
		msgindex++;
	}

	return key;
}


int main()
{
	int fd, rfd, i;
	int clilen;
	struct sockaddr client_addr;
	struct timeval tv;

	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	rfd = openserdev();
	
	displayinit(rfd);
	displaytime(rfd);
	
	if (setupSocket() == -1)
	{
		printf("Can't init socket.\n");
		return -1;
	}

	/*Become a daemon process
	*/

	ipid=fork();
	if (ipid<0) 
		exit(1);
	if (ipid>0) 
		exit(0); 

	setsid(); 
	
	i=open("/dev/null",O_RDWR); 
	dup(i); 
	dup(i); 

	signal(SIGPIPE, SIG_IGN);

	clilen=sizeof(client_addr);
	
	while (1)
	{
		/*Wait until someone connects*/
		fd=accept(sockfd,(struct sockaddr *)&client_addr,&clilen);
		if(fd==-1) 
		{
			// Unfortunately this is deadly right now.
			return -1;
		}
		else
		{
			clients[0] = fd;
			num_clients++;
			
			nolinger(fd);
		}

		/*This is the real duty cycle loop*/

		while(num_clients)
		{
			int n,retval;
			fd_set fds;
			
			FD_ZERO(&fds);

			FD_SET(rfd, &fds);
			n=rfd;
       			
			if(num_clients < 16)
			{
				FD_SET(sockfd, &fds);
				n = max(sockfd,n);
			}

			retval = select(n+1, &fds, NULL, NULL, &tv);
			if (retval == -1)
			{
				perror("select()");
			}
           	else if (retval)
			{
				if( FD_ISSET(sockfd, &fds))
				{
					int nfd;
					nfd=accept(sockfd,(struct sockaddr *)&client_addr,&clilen);
					if(nfd==-1) 
					{
						//hello ??
					}
					else
					{
						int a=0;
						for (a=0 ; a<16 ; a++)
							if ( clients[a] == -1 )
							{
								clients[a]=nfd;
								break;
							}
						num_clients++;
					}
				}
				if (FD_ISSET(rfd, &fds))
				{
					char *message;
					
					int key;
					key = getmsg(rfd);
					
					if (key >= 0)
					{
//						printf("got key: 0x%04x\n",key);
						message = getlircmessage(key);
	
						if (message)
						{
							sendlircmsg(clients, message);
							tv.tv_sec = 0;
							tv.tv_usec = 100000;
						}
					}

				}
			}
			else
			{
				tv.tv_sec = 2;
				tv.tv_usec = 0;
				/* Timeout: ignore*/
			}
			
			/* Update the display */
			displayupdate(rfd);
			
		}
	}
	return 0;
}

