#ifndef HMC5883_H
#define HMC5883_H

#include "msensor.h"

/*!
 * @class Hmc5883
 * @brief: Магнитометр Hmc5883
*/
class Hmc5883 : public MSensor
{
public:
	virtual void init() override;
	virtual void getMagn(float &x, float &y, float &z) override;
	virtual void getAcc(float &x, float &y, float &z) override;

	Hmc5883(int i2cbus);
	~Hmc5883();
};

#endif // HMC5883_H