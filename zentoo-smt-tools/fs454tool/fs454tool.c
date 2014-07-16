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

#define FS453_I2C_DEV	"/dev/i2c-1"

#define VIDEO_ENCODER_PAL 	1

/* I2C address of the FS453 chip */

#define I2C1_BUS				0
#define FS453_I2C_ADDR			0x6A

/*!
 *
 * FS453 register file
 *
 */
#define FS453_IHO				0x00	/*! Input Horizontal Offset */
#define FS453_IVO				0x02	/*! Input Vertical Offset */
#define FS453_IHW				0x04	/*! Input Horizontal Width */
#define FS453_VSC				0x06	/*! Vertical Scaling Coefficient */
#define FS453_HSC				0x08	/*! Horizontal Scaling Coefficient */
#define FS453_BYPASS			0x0A	/*! BYPASS */
#define FS453_CR				0x0C	/*! Command Register */
#define FS453_MISC				0x0E	/*! Miscellaneous Bits Register */
#define FS453_NCON				0x10	/*! Numerator of NCO Word */
#define FS453_NCOD				0x14	/*! Denominator of NCO Word */
#define FS453_PLL_M_PUMP		0x18	/*! PLL M and Pump Control */
#define FS453_PLL_N				0x1A	/*! PLL N */
#define FS453_PLL_PDIV			0x1C	/*! PLL Post-Divider */
#define FS453_SHP				0x24	/*! Sharpness Filter */
#define FS453_FLK				0x26	/*! Filcker Filter Coefficient */
#define FS453_GPIO				0x28	/*! General Purpose I/O, Output Enab */
#define FS453_ID				0x32	/*! Part Identification Number */
#define FS453_STATUS			0x34	/*! Status Port */
#define FS453_FIFO_SP			0x36	/*! FIFO Status Port Fill/Underrun */
#define FS453_FIFO_LAT			0x38	/*! FIFO Latency */
#define FS453_CHR_FREQ			0x40	/*! Chroma Subcarrier Frequency */
#define FS453_CHR_PHASE			0x44	/*! Chroma Phase */
#define FS453_MISC_45			0x45	/*! Miscellaneous Bits Register 45 */
#define FS453_MISC_46			0x46	/*! Miscellaneous Bits Register 46 */
#define FS453_MISC_47			0x47	/*! Miscellaneous Bits Register 47 */
#define FS453_HSYNC_WID			0x48	/*! HSync Width */
#define FS453_BURST_WID			0x49	/*! Burst Width */
#define FS453_BPORCH			0x4A	/*! Back Porch Width */
#define FS453_CB_BURST			0x4B	/*! Cb Burst Amplitude */
#define FS453_CR_BURST			0x4C	/*! Cr Burst Amplitude */
#define FS453_MISC_4D			0x4D	/*! Miscellaneous Bits Register 4D */
#define FS453_BLACK_LVL			0x4E	/*! Black Level */
#define FS453_BLANK_LVL			0x50	/*! Blank Level */
#define FS453_NUM_LINES			0x57	/*! Number of Lines */
#define FS453_WHITE_LVL			0x5E	/*! White Level */
#define FS453_CB_GAIN			0x60	/*! Cb Color Saturation */
#define FS453_CR_GAIN			0x62	/*! Cr Color Saturation */
#define FS453_TINT				0x65	/*! Tint */
#define FS453_BR_WAY			0x69	/*! Width of Breezeway */
#define FS453_FR_PORCH			0x6C	/*! Front Porch */
#define FS453_NUM_PIXELS		0x71	/*! Total num. of luma/chroma Pixels */
#define FS453_1ST_LINE			0x73	/*! First Video Line */
#define FS453_MISC_74			0x74	/*! Miscellaneous Bits Register 74 */
#define FS453_SYNC_LVL			0x75	/*! Sync Level */
#define FS453_VBI_BL_LVL		0x7C	/*! VBI Blank Level */
#define FS453_SOFT_RST			0x7E	/*! Encoder Soft Reset */
#define FS453_ENC_VER			0x7F	/*! Encoder Version */
#define FS453_WSS_CONFIG		0x80	/*! WSS Configuration Register */
#define FS453_WSS_CLK			0x81	/*! WSS Clock */
#define FS453_WSS_DATAF1		0x83	/*! WSS Data Field 1 */
#define FS453_WSS_DATAF0		0x86	/*! WSS Data Field 0 */
#define FS453_WSS_LNF1			0x89	/*! WSS Line Number Field 1 */
#define FS453_WSS_LNF0			0x8A	/*! WSS Line Number Field 0 */
#define FS453_WSS_LVL			0x8B	/*! WSS Level */
#define FS453_MISC_8D			0x8D	/*! Miscellaneous Bits Register 8D */
#define FS453_VID_CNTL0			0x92	/*! Video Control 0 */
#define FS453_HD_FP_SYNC		0x94	/*! Horiz. Front Porch & HSync Width */
#define FS453_HD_YOFF_BP		0x96	/*! HDTV Lum. Offset & Back Porch */
#define FS453_SYNC_DL			0x98	/*! Sync Delay Value */
#define FS453_LD_DET			0x9C	/*! DAC Load Detect */
#define FS453_DAC_CNTL			0x9E	/*! DAC Control */
#define FS453_PWR_MGNT			0xA0	/*! Power Management */
#define FS453_RED_MTX			0xA2	/*! RGB to YCrCb Matrix Red Coeff. */
#define FS453_GRN_MTX			0xA4	/*! RGB to YCrCb Matrix Green Coeff. */
#define FS453_BLU_MTX			0xA6	/*! RGB to YCrCb Matrix Blue Coeff. */
#define FS453_RED_SCL			0xA8	/*! RGB to YCrCb Scaling Red Coeff. */
#define FS453_GRN_SCL			0xAA	/*! RGB to YCrCb Scaling Green Coeff. */
#define FS453_BLU_SCL			0xAC	/*! RGB to YCrCb Scaling Blue Coeff. */
#define FS453_CC_FIELD_1		0xAE	/*! Closed Caption Field 1 Data */
#define FS453_CC_FIELD_2		0xB0	/*! Closed Caption Field 2 Data */
#define FS453_CC_CONTROL		0xB2	/*! Closed Caption Control */
#define FS453_CC_BLANK_VALUE	0xB4	/*! Closed Caption Blanking Value */
#define FS453_CC_BLANK_SAMPLE	0xB6	/*! Closed Caption Blanking Sample */
#define FS453_HACT_ST			0xB8	/*! HDTV Horizontal Active Start */
#define FS453_HACT_WD			0xBA	/*! HDTV Horizontal Active Width */
#define FS453_VACT_ST			0xBC	/*! HDTV Veritical Active Width */
#define FS453_VACT_HT			0xBE	/*! HDTV Veritical Active Height */
#define FS453_PR_PB_SCALING		0xC0	/*! Pr and Pb Relative Scaling */
#define FS453_LUMA_BANDWIDTH	0xC2	/*! Luminance Frequency Response */
#define FS453_QPR				0xC4	/*! Quick Program Register */

/*! Command register bits */

#define CR_GCC_CK_LVL			0x2000	/*! Graphics Controller switching lev */
#define CR_P656_LVL				0x1000	/*! Pixel Port Output switching level */
#define CR_P656_IN				0x0800	/*! Pixel Port In */
#define CR_P656_OUT				0x0400	/*! Pixel Port Out */
#define CR_CBAR_480P			0x0200	/*! 480P Color Bars */
#define CR_PAL_NTSCIN			0x0100	/*! PAL or NTSC input */
#define CR_SYNC_MS				0x0080	/*! Sync Master or Slave */
#define CR_FIFO_CLR				0x0040	/*! FIFO Clear */
#define CR_CACQ_CLR				0x0020	/*! CACQ Clear */
#define CR_CDEC_BP				0x0010	/*! Chroma Decimator Bypass */
#define CR_NCO_EN				0x0002	/*! Enable NCO Latch */
#define CR_SRESET				0x0001	/*! Soft Reset */

/*! Chip ID register bits */

#define FS453_CHIP_ID		0xFE05	/*! Chip ID register expected value */


struct fs453_presets {
	__u32 mode;		/*! Video mode */
	__u16 qpr;		/*! Quick Program Register */
	__u16 pwr_mgmt;		/*! Power Management */
	__u16 iho;		/*! Input Horizontal Offset */
	__u16 ivo;		/*! Input Vertical Offset */
	__u16 ihw;		/*! Input Horizontal Width */
	__u16 vsc;		/*! Vertical Scaling Coefficient */
	__u16 hsc;		/*! Horizontal Scaling Coefficient */
	__u16 bypass;		/*! Bypass */
	__u16 misc;		/*! Miscellaneous Bits Register */
	__u8 misc46;		/*! Miscellaneous Bits Register 46 */
	__u8 misc47;		/*! Miscellaneous Bits Register 47 */
	__u32 ncon;		/*! Numerator of NCO Word */
	__u32 ncod;		/*! Denominator of NCO Word */
	__u16 pllm;		/*! PLL M and Pump Control */
	__u16 plln;		/*! PLL N */
	__u16 pllpd;		/*! PLL Post-Divider */
	__u16 vid_cntl0;	/*! Video Control 0 */
	__u16 dac_cntl;		/*! DAC Control */
	__u16 fifo_lat;		/*! FIFO Latency */
};

static struct fs453_presets fs453_pal_presets = {
	.mode = VIDEO_ENCODER_PAL,
	.qpr = 0x9843,
	.pwr_mgmt = 0x0200,
	.misc = 0x0030,
	.ncon = 0x00000001,
	.ncod = 0x00000001,
	.misc46 = 0x8d,
	.misc47 = 0x08,
	.pllm = 0x7260,
	.plln = 0x007c,
	.pllpd = ((10 - 6) << 8) | (10 - 6),
	.iho = 0x0064,
	.ivo = 0x001a,
	.ihw = 0x02d0,
	.vsc = 0x0000,
	.hsc = 0x0000,
	.bypass = 0x0000,
	.vid_cntl0 = 0x0b01,
	.dac_cntl = 0x00e4,
	.fifo_lat = 0x0082,
};

/*!
 * @brief Function to read TV encoder registers on the i2c bus
 * @param     client	I2C client structure
 * @param     reg	The register number
 * @param     value	Pointer to buffer to receive the read data
 * @param     len	Number of 16-bit register words to read
 * @return    0 on success, others on failure
 */
static int fs453_read(int client, __u8 reg, __u32 * value, __u32 len)
{
	if (len == 1)
		*value = i2c_smbus_read_byte_data(client, reg);
	else if (len == 2)
		*value = i2c_smbus_read_word_data(client, reg);
	else if (len == 4) {
		*(__u16 *) value = i2c_smbus_read_word_data(client, reg);
		*((__u16 *) value + 1) =
		    i2c_smbus_read_word_data(client, reg + 2);
	} else
		return -EINVAL;

	return 0;
}

/*!
 * @brief Function to write a TV encoder register on the i2c bus
 * @param     client	I2C client structure
 * @param     reg	The register number
 * @param     value	The value to write
 * @param     len	Number of words to write (must be 1)
 * @return    0 on success, others on failure
 */
static int fs453_write(int client, __u8 reg, __u32 value, __u32 len)
{
	int tries = 5;
	int ret;
	
	if (len == 1) {
		while (tries--) {
			ret = i2c_smbus_write_byte_data(client, reg, (__u8) value);
			if (!ret)
				break;
		}
		if (ret)
			printf("FS453: bailed out writing to 0x%04x\n", reg);
		return ret;
	}
	else if (len == 2){
		while (tries--) {
			ret = i2c_smbus_write_word_data(client, reg, (__u16) value);
			if (!ret)
				break;
		}
		if (ret)
			printf("FS453: bailed out writing to 0x%04x\n", reg);
		return ret;
	}
	else if (len == 4){
		while (tries--) {
			ret = i2c_smbus_write_block_data(client, reg, len,
						  (__u8 *) & value);
			if (!ret)
				break;
		}
		if (ret)
			printf("FS453: bailed out writing to 0x%04x\n", reg);
		return ret;
	}
	else
		return -EINVAL;
}

/*!
 * @brief Function to initialize the TV encoder
 * @param     client	I2C client structure
 * @param     presets	FS453 pre-defined register values
 * @return    0 on success; ENODEV if the encoder wasn't found
 */
static int fs453_preset(int client,
			struct fs453_presets *presets)
{
	__u32 data;

	if (!client)
		return -ENODEV;

	/* set the clock level */
	fs453_write(client, FS453_CR, CR_GCC_CK_LVL, 2);

	/* soft reset the encoder */
	fs453_read(client, FS453_CR, &data, 2);
	printf("Data: %04x",data);
	fs453_write(client, FS453_CR, data | CR_SRESET, 2);
	fs453_write(client, FS453_CR, data & ~CR_SRESET, 2);

	fs453_write(client, FS453_BYPASS, presets->bypass, 2);

	/* Write the QPR (Quick Programming Register). */
	fs453_write(client, FS453_QPR, presets->qpr, 2);

	/* set up the NCO and PLL */
	fs453_write(client, FS453_NCON, presets->ncon, 4);
	fs453_write(client, FS453_NCOD, presets->ncod, 4);
	fs453_write(client, FS453_PLL_M_PUMP, presets->pllm, 2);
	fs453_write(client, FS453_PLL_N, presets->plln, 2);
	fs453_write(client, FS453_PLL_PDIV, presets->pllpd, 2);

	/* latch the NCO and PLL settings */
	fs453_read(client, FS453_CR, &data, 2);
	fs453_write(client, FS453_CR, data | CR_NCO_EN, 2);
	fs453_write(client, FS453_CR, data & ~CR_NCO_EN, 2);

	/* customize */
	fs453_write(client, FS453_PWR_MGNT, presets->pwr_mgmt, 2);

	fs453_write(client, FS453_IHO, presets->iho, 2);
	fs453_write(client, FS453_IVO, presets->ivo, 2);
	fs453_write(client, FS453_IHW, presets->ihw, 2);
	fs453_write(client, FS453_VSC, presets->vsc, 2);
	fs453_write(client, FS453_HSC, presets->hsc, 2);

	fs453_write(client, FS453_MISC, presets->misc, 2);

	fs453_write(client, FS453_VID_CNTL0, presets->vid_cntl0, 2);
	fs453_write(client, FS453_MISC_46, presets->misc46, 1);
	fs453_write(client, FS453_MISC_47, presets->misc47, 1);

	fs453_write(client, FS453_DAC_CNTL, presets->dac_cntl, 2);
	fs453_write(client, FS453_FIFO_LAT, presets->fifo_lat, 2);
	
	return 0;
}

int memfd		= -1;
unsigned char *mmio	= NULL;
off_t pciaddr		= 0xf4000000;

#define i810_readb(mmio_base, where)                     \
        *((volatile char *) (mmio_base + where))

#define i810_readw(mmio_base, where)                     \
	*((volatile short *) (mmio_base + where))

#define i810_readl(mmio_base, where)                     \
	*((volatile int *) (mmio_base + where))

#define i810_writeb(mmio_base, where, val)                              \
	*((volatile char *) (mmio_base + where)) = (volatile char) val

#define i810_writew(mmio_base, where, val)                              \
	*((volatile short *) (mmio_base + where)) = (volatile short) val

#define i810_writel(mmio_base, where, val)                              \
	*((volatile int *) (mmio_base + where)) = (volatile int) val

void memClose(void) 
{
	if (mmio) 
		munmap(mmio, 512 * 1024);
	if (memfd >= 0) 
		close(memfd);
	memfd	= -1;
	mmio	= NULL;
}

void memOpen(void) 
{
	memfd	= open("/dev/mem", O_RDWR);
	if (memfd < 0) {
		int i	= errno;
		perror("/dev/mem");
		if (i == EACCES && geteuid() != 0)
			fprintf(stderr, "Must have access to `/dev/mem'.\n");
		memClose();
		exit(4);
	}

	mmio	= mmap(NULL, 512 * 1024, PROT_WRITE | PROT_READ, MAP_SHARED, memfd, pciaddr);
	if (!mmio) {
		memClose();
		exit(5);
	}
}

/* i810 */
/* LCD/TV-Out and HW DVD Registers (60000h 6FFFFh) */
/* LCD/TV-Out */
#define HTOTAL                0x60000
#define HBLANK                0x60004
#define HSYNC                 0x60008
#define VTOTAL                0x6000C
#define VBLANK                0x60010
#define VSYNC                 0x60014
#define LCDTV_C               0x60018
#define OVRACT                0x6001C
#define BCLRPAT               0x60020

void i815_setup(void) {
	memOpen();

		i810_writel(mmio, HTOTAL,  0x035f02cf); //HTOTAL
		i810_writel(mmio, HBLANK,  0x035f02cf); //HBLANK
		i810_writel(mmio, HSYNC,   0x033702f7); //HSYNC
		i810_writel(mmio, VTOTAL,  0x0270023f); //VTOTAL
		i810_writel(mmio, VBLANK,  0x0270023f); //VBLANK
		i810_writel(mmio, VSYNC,   0x02580256); //VSYNC
		i810_writel(mmio, LCDTV_C, 0xa0004047); //LCDTV_C
		i810_writel(mmio, OVRACT,  0x02af033f); //OVRACT
		i810_writel(mmio, BCLRPAT, 0x00000000); //BCLRPAT
	memClose();
}

int main () 
{
	int client = open(FS453_I2C_DEV, O_RDWR);
	ioctl(client, I2C_SLAVE_FORCE, FS453_I2C_ADDR);

	fs453_write(client, FS453_PWR_MGNT, 0x3BFF, 2);

	i815_setup();
	fs453_preset(client, &fs453_pal_presets);
}
