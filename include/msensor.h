#ifndef MSENSOR_H
#define MSENSOR_H

#include "i2c.h"

/*!
 * @def ACC_VAL_MAX
 * @brief: Максимальное значение измерения акселерации
*/
#define ACC_VAL_MAX 16384
/*!
 * @def AccNorm
 * @brief: Нормализация значения акселерации
*/
#define AccNorm(x) \
	if (x < -ACC_VAL_MAX) x = -ACC_VAL_MAX; \
	if (x > ACC_VAL_MAX) x = ACC_VAL_MAX;
	
#define shift16(val, sh) \
	(uint16_t)(((uint16_t)val) << sh)
#define sh16_msb(buf) \
	shift16(((buf)[1]), 0) | shift16(((buf)[0]), 8)
#define sh16_lsb(buf) \
	shift16(((buf)[0]), 0) | shift16(((buf)[1]), 8)

/*!
 * @class MSensor
 * @brief: Абстрактный класс магнитометра
*/
class MSensor {
public:
	/*!
	* @fn MSensor::init
	* @brief: ининциализация датчика
	*/
	virtual void init() = 0;
	/*!
	* @fn MSensor::getMagn
	* @brief: получение показаний магнитного поля
	*/
	virtual void getMagn(float &x, float &y, float &z) = 0;
	/*!
	* @fn MSensor::getAcc
	* @brief: получение показаний акселерации
	*/
	virtual void getAcc(float &x, float &y, float &z) = 0;

	union uni16_t {
		uint16_t u;
		int16_t i;
	};

	MSensor(int i2cbus, int address) {
		i2c = new I2c(address, i2cbus);
		x = y = z = 0.0f;
		ax = ay = az = 0.0f;
		rng = 0.0f;
	}
	virtual ~MSensor() {
		delete i2c;
	}

protected:
	I2c *i2c;			//!< шина i2c
	float x, y, z;		//!< показания магнитного поля
	float ax, ay, az;	//!< показания акселерации
	float rng;			//!< коэффициент усиления магнитных измерений
	float as;			//!< коэффициент усиления измерений акселерации 
};

#endif // MSENSOR_H