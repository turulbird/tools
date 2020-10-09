/*
 * eeprom.c - Version for Kathrein UFS913
 *
 * (c) 2011 konfetti
 * partly copied from uboot source!
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * Description:
 *
 * Little utility to dump the EEPROM contents or show the
 * MAC address on Fortis receivers.
 * In effect this a stripped down version of ipbox_eeprom with minor
 * modifications and additions. Kudos to the original authors.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CFG_EEPROM_ADDR  0x57
#define CFG_EEPROM_SIZE  256

//#define EEPROM_WRITE
#if defined EEPROM_WRITE
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	11	/* 10ms. but give more */
#define CFG_EEPROM_PAGE_WRITE_BITS 4
#define	EEPROM_PAGE_SIZE  (1 << CFG_EEPROM_PAGE_WRITE_BITS)
#endif

/* ********************************* i2c ************************************ */
/*
 * I2C Message - used for pure i2c transaction, also from /dev interface
 */
struct i2c_msg
{
	unsigned short addr;    /* slave address       */
	unsigned short flags;
	unsigned short len;     /* msg length          */
	unsigned char  *buf;    /* pointer to msg data */
};

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data
{
	struct i2c_msg *msgs;   /* pointers to i2c_msgs */
	unsigned int nmsgs;     /* number of i2c_msgs   */
};


#define I2C_SLAVE 0x0703    /* Change slave address */
/* Attn.: Slave address is 7 or 10 bits */

#define I2C_RDWR 0x0707     /* Combined R/W transfer (one stop only) */

int i2c_read(int fd_i2c, unsigned char addr, unsigned char reg, unsigned char *buffer, int len)
{
	struct i2c_rdwr_ioctl_data i2c_rdwr;
	int	err;
	unsigned char b0[] = { reg };

	//printf("%s > i2c addr = 0x%02x, reg = 0x%02x, len = %d\n", __func__, addr, reg, len);

	i2c_rdwr.nmsgs = 2;
	i2c_rdwr.msgs = malloc(2 * sizeof(struct i2c_msg));
	i2c_rdwr.msgs[0].addr = addr;
	i2c_rdwr.msgs[0].flags = 0;
	i2c_rdwr.msgs[0].len = 1;
	i2c_rdwr.msgs[0].buf = b0;

	i2c_rdwr.msgs[1].addr = addr;
	i2c_rdwr.msgs[1].flags = 1;
	i2c_rdwr.msgs[1].len = len;
	i2c_rdwr.msgs[1].buf = malloc(len);

	memset(i2c_rdwr.msgs[1].buf, 0, len);

	if ((err = ioctl(fd_i2c, I2C_RDWR, &i2c_rdwr)) < 0)
	{
		printf("[eeprom] %s i2c_read of reg 0x%02x failed %d %d\n", __func__, reg, err, errno);
		printf("         %s\n", strerror(errno));
		free(i2c_rdwr.msgs[0].buf);
		free(i2c_rdwr.msgs);
		return -1;
	}

	//printf("[eeprom] %s reg 0x%02x -> ret 0x%02x\n", __func__, reg, (i2c_rdwr.msgs[1].buf[0] & 0xff));
	memcpy(buffer, i2c_rdwr.msgs[1].buf, len);

	free(i2c_rdwr.msgs[1].buf);
	free(i2c_rdwr.msgs);

	//printf("[eeprom] %s <\n", __func__);
	return 0;
}

#if defined EEPROM_WRITE
int i2c_write(int fd_i2c, unsigned char addr, unsigned char reg, unsigned char *buffer, int len)
{
	struct 	i2c_rdwr_ioctl_data i2c_rdwr;
	int	err;
	unsigned char buf[256];

	//printf("%s > 0x%0x - %s - %d\n", __func__, reg, buffer, len);

	buf[0] = reg;
	memcpy(&buf[1], buffer, len);

	i2c_rdwr.nmsgs = 1;
	i2c_rdwr.msgs = malloc(1 * sizeof(struct i2c_msg));
	i2c_rdwr.msgs[0].addr = addr;
	i2c_rdwr.msgs[0].flags = 0;
	i2c_rdwr.msgs[0].len = len + 1;
	i2c_rdwr.msgs[0].buf = buf;

	if ((err = ioctl(fd_i2c, I2C_RDWR, &i2c_rdwr)) < 0)
	{
		//printf("i2c_read failed %d %d\n", err, errno);
		//printf("%s\n", strerror(errno));
		free(i2c_rdwr.msgs[0].buf);
		free(i2c_rdwr.msgs);
		return -1;
	}
	free(i2c_rdwr.msgs);
//	printf("%s <\n", __func__);
	return 0;
}
#endif

/* *************************** uboot copied func ************************************** */

#if defined EEPROM_WRITE
int eeprom_write(int fd, unsigned dev_addr, unsigned offset, unsigned char *buffer, unsigned cnt)
{
	unsigned end = offset + cnt;
	unsigned blk_off;
	int rcode = 0;

	/* Write data until done or would cross a write page boundary.
	 * We must write the address again when changing pages
	 * because the address counter only increments within a page.
	 */

	printf("[eeprom] %s >\n", __func__);
	while (offset < end)
	{
		unsigned len;
		unsigned char addr[2];

		blk_off = offset & 0xFF;    /* block offset */

		addr[0] = offset >> 8;      /* block number */
		addr[1] = blk_off;          /* block offset */
		addr[0] |= dev_addr;        /* insert device address */

		len = end - offset;

		if (i2c_write(fd, addr[0], offset, buffer, len) != 0)
		{
			rcode = 1;
		}
		buffer += len;
		offset += len;

		usleep(CFG_EEPROM_PAGE_WRITE_DELAY_MS * 1000);
	}
	printf("[eeprom] %s < (%d)\n", __func__, rcode);
	return rcode;
}
#endif

int eeprom_read(int fd, unsigned dev_addr, unsigned offset, unsigned char *buffer, unsigned cnt)
{
	unsigned end = offset + cnt;
	unsigned blk_off;
	int rcode = 0;
	int i;

	//printf("[eeprom] %s > offset 0x%03x, count %d bytes\n", __func__, offset, cnt);

	while (offset < end)
	{
		unsigned len;
		unsigned char addr[2];

		blk_off = offset & 0xFF;    /* block offset */

		addr[0] = offset >> 8;      /* block number */
		addr[1] = blk_off;          /* block offset */
		addr[0] |= dev_addr;        /* insert device address */

		len = cnt;
		//printf("[eeprom] %s address=0x%02x, reg=0x%02x\n", __func__, addr[0], addr[1]);

		for (i = 0; i < len; i++)
		{
			if (i2c_read(fd, addr[0], offset + i, &buffer[i], 1) != 0)
			{
				rcode = 1;
				break;
			}
		}
		buffer += len;
		offset += len;
	}
	//printf("[eeprom] %s < (%d)\n", __func__, rcode);
	return rcode;
}

int main(int argc, char *argv[])
{
	int fd_i2c;
	int vLoop;
	int i, rcode = 0;
	unsigned char buf[CFG_EEPROM_SIZE];

//	printf("%s >\n", argv[0]);

	fd_i2c = open("/dev/i2c-0", O_RDWR);

	rcode |= eeprom_read(fd_i2c, CFG_EEPROM_ADDR, 0, buf, sizeof(buf));
	if (rcode < 0)
	{
		rcode = 1;
		goto failed;
	}
	printf("EEPROM dump\n");
	printf("Addr  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	printf("-----------------------------------------------------\n");
	for (vLoop = 0; vLoop < CFG_EEPROM_SIZE; vLoop += 0x10)
	{
		printf(" %02x ", vLoop);
		for (i = 0; i < 0x10; i++)
		{
			printf(" %02x", buf[vLoop + i]);
		}
		printf("\n");
	}
	return 0;

failed:
	printf("[eeprom] failed <\n");
	return -1;
}
// vim:ts=4
