#ifndef QMC5883L_H
#define QMC5883L_H

#include "msensor.h"

/*!
 * @class Qmc5883l
 * @brief: Магнитометр Qmc5883l
*/
class Qmc5883l : public MSensor
{
public:
	virtual void init() override;
	virtual void getMagn(float &x, float &y, float &z) override;
	virtual void getAcc(float &x, float &y, float &z) override;

	Qmc5883l(int i2cbus);
	~Qmc5883l();
};

#endif