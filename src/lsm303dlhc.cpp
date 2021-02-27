#include "lsm303dlhc.h"

#include <unistd.h>

#define AADDRESS 0x19
#define MADDRESS 0x1e

enum Reg {
	// magnetic
	cra,
	crb,
	mr,
	x_msb,
	x_lsb,
	z_msb,
	z_lsb,
	y_msb,
	y_lsb,
	status,
	ira,
	irb,
	irc,

	// acceleration
	#if 1
	ctrl1 = 0x20,
	ctrl2,
	ctrl3,
	ctrl4,
	ctrl5,
	ctrl6,
	ref,
	astat,
	ax_lsb,
	ax_msb,
	ay_lsb,
	ay_msb,
	az_lsb,
	az_msb,
	fifo_ctrl,
	fifo_src,
	int1_cfg,
	int1_src,
	int1_ths,
	int1_dur,
	int2_cfg,
	int2_src,
	int2_ths,
	int2_dur,
	click_cfg,
	click_src,
	click_ths,
	time_lim,
	time_lat,
	time_wind
	#endif
};

#define CRA_DO_MASK	0x1c
#define CRA_TS_EN	0x80

#define CRB_GN_MASK	0xe0

#define MR_MD_MASK	0x03

#define STAT_DRDY_MASK	0x01
#define STAT_LOCK_MASK	0x02

// cra
#define DO_SHIFT		2
#define DO_0_75			(0x00 << DO_SHIFT)
#define DO_1_5			(0x01 << DO_SHIFT)
#define DO_3			(0x02 << DO_SHIFT)
#define DO_7_5			(0x03 << DO_SHIFT)
#define DO_15			(0x04 << DO_SHIFT)
#define DO_30			(0x05 << DO_SHIFT)
#define DO_75			(0x06 << DO_SHIFT)
#define DO_220			(0x07 << DO_SHIFT)
#define TS_EN_SHIFT		7
#define TS_EN_OFF		(0x00 << TS_EN_SHIFT)
#define TS_EN_ON		(0x01 << TS_EN_SHIFT)

// crb
#define GN_SHIFT		5
#define GN_1_3			(0x01 << GN_SHIFT)
#define GN_1_9			(0x02 << GN_SHIFT)
#define GN_2_5			(0x03 << GN_SHIFT)
#define GN_4			(0x04 << GN_SHIFT)
#define GN_4_7			(0x05 << GN_SHIFT)
#define GN_5_6			(0x06 << GN_SHIFT)
#define GN_8_1			(0x07 << GN_SHIFT)

#define DIGRES(x) (float)((1.0f/(float)x)*1000.0f)
#define RNG_1_3			DIGRES(1100)
#define RNG_1_9			DIGRES(855)
#define RNG_2_5			DIGRES(670)
#define RNG_4			DIGRES(450)
#define RNG_4_7			DIGRES(400)
#define RNG_5_6			DIGRES(330)
#define RNG_8_1			DIGRES(230)

// mode
#define MD_SHIFT	0
#define MD_CONT		(0x00 << MD_SHIFT)
#define MD_SINGLE	(0x01 << MD_SHIFT)
#define MD_IDLE		(0x02 << MD_SHIFT)

// ctrl1
#define XEN_SHIFT		0
#define XEN_OFF			(0x0 << XEN_SHIFT)
#define XEN_ON			(0x1 << XEN_SHIFT)
#define YEN_SHIFT		1
#define YEN_OFF			(0x0 << YEN_SHIFT)
#define YEN_ON			(0x1 << YEN_SHIFT)
#define ZEN_SHIFT		2
#define ZEN_OFF			(0x0 << ZEN_SHIFT)
#define ZEN_ON			(0x1 << ZEN_SHIFT)
#define LPEN_SHIFT		3
#define LPEN_OFF		(0x0 << LPEN_SHIFT)
#define LPEN_ON			(0x1 << LPEN_SHIFT)
#define ODR_SHIFT		4
#define ODR_OFF			(0x0 << ODR_SHIFT)
#define ODR_1HZ			(0x1 << ODR_SHIFT)
#define ODR_10HZ		(0x2 << ODR_SHIFT)
#define ODR_25HZ		(0x3 << ODR_SHIFT)
#define ODR_50HZ		(0x4 << ODR_SHIFT)
#define ODR_100HZ		(0x5 << ODR_SHIFT)
#define ODR_200HZ		(0x6 << ODR_SHIFT)
#define ODR_400HZ		(0x7 << ODR_SHIFT)
#define ODR_1620HZ		(0x8 << ODR_SHIFT)
#define ODR_1344HZ		(0x9 << ODR_SHIFT)

// ctrl2
#define HPIS1_SHIFT		0
#define HPIS1_OFF		(0x0 << HPIS1_SHIFT)
#define HPIS1_ON		(0x1 << HPIS1_SHIFT)
#define HPIS2_SHIFT		1
#define HPIS2_OFF		(0x0 << HPIS2_SHIFT)
#define HPIS2_ON		(0x1 << HPIS2_SHIFT)
#define HPCLICK_SHIFT	2
#define HPCLICK_OFF		(0x0 << HPCLICK_SHIFT)
#define HPCLICK_ON		(0x1 << HPCLICK_SHIFT)
#define FDS_SHIFT		2
#define FDS_OFF			(0x0 << FDS_SHIFT)
#define FDS_ON			(0x1 << FDS_SHIFT)
#define HPCF_SHIFT		3
#define HPM_SHIFT		5
#define HPM_NORM		(0x0 << HPM_SHIFT)
#define HPM_REFS		(0x1 << HPM_SHIFT)
#define HPM_NORM2		(0x2 << HPM_SHIFT)
#define HPM_AUTO		(0x3 << HPM_SHIFT)

// ctrl3

// ctrl4
#define SIM_SHIFT		0
#define HR_SHIFT		3
#define HR_OFF			(0x0 << HR_SHIFT)
#define HR_ON			(0x1 << HR_SHIFT)
#define FS_SHIFT		4
#define FS_2			(0x0 << FS_SHIFT)
#define FS_4			(0x1 << FS_SHIFT)
#define FS_8			(0x2 << FS_SHIFT)
#define FS_16			(0x3 << FS_SHIFT)
#define BLE_SHIFT		6
#define BLE_LSB			(0x0 << BLE_SHIFT)
#define BLE_MSB			(0x1 << BLE_SHIFT)
#define BDU_SHIFT		7
#define BDU_CONT		(0x0 << BDU_SHIFT)
#define BDU_ONREAD		(0x1 << BDU_SHIFT)

#define LSM_AS_2g		1.0f
#define LSM_AS_4g		0.5f
#define LSM_AS_8g		0.25f
#define LSM_AS_16g		0.0834f

// stat
#define ZYXDA_MASK		0x8


void Lsm303dlhc::getAcc(float &x, float &y, float &z)
{
	uint8_t buf[16];
	uni16_t uni;
	if (accel->read(Reg::astat, buf+0, 1) == 1) {
		if (buf[0] & ZYXDA_MASK) {
			if (accel->read(Reg::ax_lsb, buf+0, 6) == 6) {
				uni.u = sh16_lsb(buf+0); this->ax = (float)uni.i;
				uni.u = sh16_lsb(buf+2); this->ay = (float)uni.i;
				uni.u = sh16_lsb(buf+4); this->az = (float)uni.i;
				this->ax /= this->as;
				this->ay /= this->as;
				this->az /= this->as;
				AccNorm(this->ax);
				AccNorm(this->ay);
				AccNorm(this->az);
			}
		}
	}
	x = this->ax;
	y = this->ay;
	z = this->az;
}


void Lsm303dlhc::getMagn(float &x, float &y, float &z)
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


void Lsm303dlhc::init()
{
	i2c->write(Reg::cra, DO_220);
	i2c->write(Reg::crb, GN_4);
	i2c->write(Reg::mr, MD_CONT);
	rng = RNG_4;

	accel->write(Reg::ctrl1, ODR_400HZ | XEN_ON | YEN_ON | ZEN_ON);
	accel->write(Reg::ctrl4, FS_2 | HR_ON);
	as = LSM_AS_2g;
}


Lsm303dlhc::Lsm303dlhc(int i2cbus) : MSensor(i2cbus, MADDRESS)
{
	accel = new I2c(AADDRESS, i2cbus);
}


Lsm303dlhc::~Lsm303dlhc()
{
	delete accel;
}