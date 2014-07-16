/*
 stv6421tool: a tool to control the SCART-OUT for the Samsung SMT 7020S Set-Top-Box
Copyright (C) 2011 D.Herrendoerfer

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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "i2c-dev.h"
#include <asm/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#define STV6421_I2C_DEV	"/dev/i2c-4"

/* I2C address of the STV6421 chip */

#define STV6421_I2C_ADDR			0x4A

static int stv_write(int client, __u8 reg, __u32 value)
{
	int tries = 5;
	int ret;
	
	while (tries--) {
		ret = i2c_smbus_write_byte_data(client, reg, (__u8) value);
		if (!ret)
			break;
	}
	if (ret)
		printf("STV6421: bailed out writing to 0x%04x\n", reg);
	return ret;
}


static int stv6421_preset_tv(int client)
{
	stv_write(client, 0x02, 0x01); // TV-video auf SCART1
	stv_write(client, 0x01, 0x11); // TV-audio auf SCART1
	stv_write(client, 0x03, 0xf6); // TV RGB&FB auf SCART1
}

static int stv6421_preset_vcr(int client)
{
	stv_write(client, 0x02, 0x03); // VCR-video auf SCART1
	stv_write(client, 0x01, 0x2a); // VCR-audio auf SCART1
	stv_write(client, 0x03, 0xfb); // VCR RGB&FB auf SCART1
}

static int stv6421_preset_power_on(int client)
{
	stv_write(client, 0x05, 0x38); // slow blanking on, d.h. scart-schaltspannung einschalten
	stv_write(client, 0x04, 0x80); // slow blanking normal
}

static int stv6421_preset_power_off(int client)
{
	stv_write(client, 0x05, 0x18); // slow blanking off, d.h. scart-schaltspannung abschalten fuer ENC
	stv_write(client, 0x04, 0xf0); // slow blanking from VCR
}

int main (int argc, char *argv[])
{
	int opt,client;

	client = open(STV6421_I2C_DEV, O_RDWR);
	ioctl(client, I2C_SLAVE_FORCE, STV6421_I2C_ADDR);

   while ((opt=getopt(argc,argv,"tvos"))!=-1){
		switch(opt){
			case 't':
			stv6421_preset_tv(client);
			break;
			case 'v':
			stv6421_preset_vcr(client);
			break;
			case 'o':
			stv6421_preset_power_on(client);
			stv6421_preset_tv(client);
			break;
			case 's':
			stv6421_preset_power_off(client);
			stv6421_preset_vcr(client);
			break;
			
        default:
			exit(0);
		}
	}

}
