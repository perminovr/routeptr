#include "i2c.h"

#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "exceptionwhat.h"


//  #include <linux/i2c-dev.h>
/* /dev/i2c-X ioctl commands.  The ioctl's parameter is always an
 * unsigned long, except for:
 *	- I2C_FUNCS, takes pointer to an unsigned long
 *	- I2C_RDWR, takes pointer to struct i2c_rdwr_ioctl_data
 *	- I2C_SMBUS, takes pointer to struct i2c_smbus_ioctl_data
 */
#define I2C_RETRIES	0x0701	/* number of times a device address should be polled when not acknowledging */
#define I2C_TIMEOUT	0x0702	/* set timeout in units of 10 ms */
#define I2C_SLAVE	0x0703	/* Use this slave address */
#define I2C_SLAVE_FORCE	0x0706	/* Use this slave address, even if it is already in use by a driver! */
#define I2C_TENBIT	0x0704	/* 0 for 7 bit addrs, != 0 for 10 bit */
#define I2C_FUNCS	0x0705	/* Get the adapter functionality mask */
#define I2C_RDWR	0x0707	/* Combined R/W transfer (one STOP only) */
#define I2C_PEC		0x0708	/* != 0 to use PEC with SMBus */
#define I2C_SMBUS	0x0720	/* SMBus transfer */
/* This is the structure as used in the I2C_SMBUS ioctl call */
struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data *data;
};
/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data {
	struct i2c_msg *msgs;	/* pointers to i2c_msgs */
	__u32 nmsgs;			/* number of i2c_msgs */
};
#define  I2C_RDWR_IOCTL_MAX_MSGS	42
/* Originally defined with a typo, keep it for compatibility */
#define  I2C_RDRW_IOCTL_MAX_MSGS	I2C_RDWR_IOCTL_MAX_MSGS
// #end include <linux/i2c-dev.h>


using namespace std;


static inline int i2c_smbus_access(int fd, uint8_t rw, uint8_t reg, uint32_t size, uint8_t &value)
{
	i2c_smbus_data data;
	data.byte = (__u8)value;
	i2c_smbus_ioctl_data args = {
		.read_write = rw,
		.command    = reg,
		.size       = size,
		.data       = &data
	};
	int ret = ioctl(fd, I2C_SMBUS, &args);
	value = (uint8_t)data.byte & 0xFF;
	return ret;
}
#define i2c_smbus_write(fd, reg, value) i2c_smbus_access(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, value)
#define i2c_smbus_read(fd, reg, value) i2c_smbus_access(fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA, value)


int I2c::read(uint8_t reg, uint8_t *buf, uint8_t size) const
{
	int res;
	uint8_t i = 0;
	for (; i < size; ++i) {
		uint8_t byte = 0;
		res = i2c_smbus_read(this->fd, reg++, byte);
		if (res < 0) {
			return -errno;
		}
		buf[i] = byte;
	}
	return i;
}


int I2c::read(uint8_t reg, uint8_t &buf) const
{
	return read(reg, &buf, 1);
}


int I2c::write(uint8_t reg, const uint8_t *buf, uint8_t size) const
{
	int res;
	uint8_t i = 0;
	for (; i < size; ++i) {
		uint8_t byte =  buf[i++];
		res = i2c_smbus_write(this->fd, reg++, byte);
		if (res < 0) {
			return -errno;
		}
	}
	return i;
}


int I2c::write(uint8_t reg, uint8_t buf) const
{
	return write(reg, &buf, 1);
}


I2c::I2c(int realDevId, int sysDevID)
{
	this->realDevId = realDevId;
	this->devName = "/dev/i2c-" + to_string(sysDevID);
	this->fd = open(this->devName.c_str(), O_RDWR);
	if (this->fd < 0) {
		goto error;
	}
	if (ioctl(fd, I2C_SLAVE, realDevId) < 0) {
		goto error;
	}
	return;

error:
	throw ExceptionWhat("Couldn't open " + this->devName + ": " + strerror(errno));
}


I2c::~I2c()
{
	close(fd);
}