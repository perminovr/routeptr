#include "hmc5883.h"

#include <unistd.h>

#define ADDRESS 0x1e

enum Reg {
	conf_a,
	conf_b,
	mode,
	x_msb,
	x_lsb,
	z_msb,
	z_lsb,
	y_msb,
	y_lsb,
	status,
	ident_a,
	ident_b,
	ident_c
};

#define CONFA_MS_MASK	0x03
#define CONFA_DO_MASK	0x1c
#define CONFA_MA_MASK	0x60

#define CONFB_GN_MASK	0xe0

#define MODE_MD_MASK	0x03
#define MODE_HS_MASK	0x80

#define STAT_DRDY_MASK	0x01
#define STAT_LOCK_MASK	0x02

// conf a
#define MS_SHIFT		0
#define MS_NORM			(0x00 << MS_SHIFT)
#define MS_POSITIVE		(0x01 << MS_SHIFT)
#define MS_NEGATIVE		(0x02 << MS_SHIFT)
#define DO_SHIFT		2
#define DO_0_75			(0x00 << DO_SHIFT)
#define DO_1_5			(0x01 << DO_SHIFT)
#define DO_3			(0x02 << DO_SHIFT)
#define DO_7_5			(0x03 << DO_SHIFT)
#define DO_15			(0x04 << DO_SHIFT)
#define DO_30			(0x05 << DO_SHIFT)
#define DO_75			(0x06 << DO_SHIFT)
#define MA_SHIFT		5
#define MA_1			(0x00 << MA_SHIFT)
#define MA_2			(0x01 << MA_SHIFT)
#define MA_4			(0x02 << MA_SHIFT)
#define MA_8			(0x03 << MA_SHIFT)

// conf b
#define GN_SHIFT		5
#define GN_0_88			(0x00 << GN_SHIFT)
#define GN_1_3			(0x01 << GN_SHIFT)
#define GN_1_9			(0x02 << GN_SHIFT)
#define GN_2_5			(0x03 << GN_SHIFT)
#define GN_4			(0x04 << GN_SHIFT)
#define GN_4_7			(0x05 << GN_SHIFT)
#define GN_5_6			(0x06 << GN_SHIFT)
#define GN_8_1			(0x07 << GN_SHIFT)

#define DIGRES(x) (float)((1.0f/(float)x)*1000.0f)
#define RNG_0_88		DIGRES(1370)
#define RNG_1_3			DIGRES(1090)
#define RNG_1_9			DIGRES(820)
#define RNG_2_5			DIGRES(660)
#define RNG_4			DIGRES(440)
#define RNG_4_7			DIGRES(390)
#define RNG_5_6			DIGRES(330)
#define RNG_8_1			DIGRES(230)

// mode
#define MD_SHIFT	0
#define MD_CONT		(0x00 << MD_SHIFT)
#define MD_SINGLE	(0x01 << MD_SHIFT)
#define MD_IDLE		(0x02 << MD_SHIFT)
#define HS_SHIFT	7
#define HS_OFF		(0x00 << HS_SHIFT)
#define HS_ON		(0x01 << HS_SHIFT)





void Hmc5883::getAcc(float &x, float &y, float &z)
{
	x = 0; y = 0; z = -ACC_VAL_MAX;
}


void Hmc5883::getMagn(float &x, float &y, float &z)
{
	uint8_t buf[16];
	uni16_t uni;
	if (i2c->read(Reg::status, buf+0, 1) == 1) {
		if (buf[0] & STAT_DRDY_MASK) {
			if (i2c->read(Reg::x_msb, buf+0, 6) == 6) {
				uni.u = sh16_msb(buf+0); this->x = (float)uni.i;
				uni.u = sh16_msb(buf+2); this->z = (float)uni.i;
				uni.u = sh16_msb(buf+4); this->y = (float)uni.i;
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


void Hmc5883::init()
{
	i2c->write(Reg::conf_a, MS_NORM | DO_15 | MA_8);
	i2c->write(Reg::conf_b, GN_4);
	i2c->write(Reg::mode, MD_CONT | HS_ON);
	rng = RNG_4;
}


Hmc5883::Hmc5883(int i2cbus) : MSensor(i2cbus, ADDRESS)
{}


Hmc5883::~Hmc5883()
{}

