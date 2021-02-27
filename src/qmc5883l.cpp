#include "qmc5883l.h"

#include <unistd.h>

#define ADDRESS 0x0d

enum Reg {
	x_lsb,
	x_msb,
	y_lsb,
	y_msb,
	z_lsb,
	z_msb,
	status,
	t_lsb,
	t_msb,
	control1,
	control2,
	period,
	chipid
};

#define STAT_DRDY_MASK 	0x01
#define STAT_OVL_MASK 	0x02
#define STAT_DOR_MASK 	0x04

#define CTL_MODE_MASK	0x03
#define CTL_ODR_MASK	0x0c
#define CTL_RNG_MASK	0x30
#define CTL_OSR_MASK	0xc0

#define CTL2_INT_MASK	0x01
#define CTL2_ROL_MASK	0x40
#define CTL2_RST_MASK	0x80

// control 1
#define MODE_SHIFT		0
#define MODE_STANDBY	(0x00 << MODE_SHIFT)
#define MODE_CONT		(0x01 << MODE_SHIFT)
#define ODR_SHIFT		2
#define ODR_10			(0x00 << ODR_SHIFT)
#define ODR_50			(0x01 << ODR_SHIFT)
#define ODR_100			(0x02 << ODR_SHIFT)
#define ODR_200			(0x03 << ODR_SHIFT)
#define RNG_SHIFT		4
#define RNG_2G			(0x00 << RNG_SHIFT)
#define RNG_8G			(0x01 << RNG_SHIFT)
#define OSR_SHIFT		6
#define OSR_512			(0x00 << OSR_SHIFT)
#define OSR_256			(0x01 << OSR_SHIFT)
#define OSR_128			(0x02 << OSR_SHIFT)
#define OSR_64			(0x03 << OSR_SHIFT)

#define PERIOD_RECOMM	0x01

#define RNG_2G_K		1.22f
#define RNG_8G_K		4.35f


void Qmc5883l::getAcc(float &x, float &y, float &z)
{
	x = 0; y = 0; z = -ACC_VAL_MAX;
}


void Qmc5883l::getMagn(float &x, float &y, float &z)
{
	uint8_t buf[16];
	uni16_t uni;
	if (i2c->read(Reg::status, buf+0, 1) == 1) {
		if (buf[0] & STAT_DRDY_MASK) {
			if (i2c->read(Reg::x_lsb, buf+0, 6) == 6) {
				uni.u = sh16_lsb(buf+0); this->x = (float)uni.i;
				uni.u = sh16_lsb(buf+2); this->y = (float)uni.i;
				uni.u = sh16_lsb(buf+4); this->z = (float)uni.i;
				this->x *= this->rng;
				this->z *= this->rng;
				this->y *= this->rng;
			}
		}
	}
	x = this->x;
	y = this->y;
	z = this->z;
}


void Qmc5883l::init()
{
	i2c->write(Reg::control1, MODE_CONT | ODR_10 | RNG_8G | OSR_512);
	i2c->write(Reg::period, PERIOD_RECOMM);
	rng = RNG_8G_K;
}


Qmc5883l::Qmc5883l(int i2cbus) : MSensor(i2cbus, ADDRESS)
{}


Qmc5883l::~Qmc5883l()
{}

