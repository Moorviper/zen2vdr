/*
 smt-ready: a tool to signal "os-ready" for the Samsung SMT 7020S Set-Top-Box
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
#include <sys/io.h>	

int main(void)
{
//	int i;
	int ret;

	ret = iopl(3);
	if (ret) {
		perror("iopl");
		return 1;
	}

//	for (i=0x1180; i < 0x11c0; i+=4) {
//		unsigned int data = inl_p(i);
//		printf("%08x\n", data);
//	}

//	outl_p(0x00cc0000, 0x118c); /* Default value */
	outl_p(0x008c0000, 0x118c);
	
	return 0;
}
