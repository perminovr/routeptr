#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include "msensor.h"

/*!
 * @class Lsm303dlhc
 * @brief: Магнитометр Lsm303dlhc
*/
class Lsm303dlhc : public MSensor
{
public:
	virtual void init() override;
	virtual void getMagn(float &x, float &y, float &z) override;
	virtual void getAcc(float &x, float &y, float &z) override;

	Lsm303dlhc(int i2cbus);
	~Lsm303dlhc();

private:
	I2c *accel;	//!< шина i2c для чтения данных акселерации (другой адрес)
};

#endif // LSM303DLHC_H