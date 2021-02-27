#ifndef I2C_H
#define I2C_H

#include <string>

/*!
 * @class I2c
 * @brief:
*/
class I2c 
{
public:
	int read(uint8_t reg, uint8_t &buf) const;
	int read(uint8_t reg, uint8_t *buf, uint8_t size = 1) const;
	int write(uint8_t reg, uint8_t buf) const;
	int write(uint8_t reg, const uint8_t *buf, uint8_t size = 1) const;

	/*!
	* @fn I2c::I2c
	* @brief: 
	* @param realDevId - адрес на шине i2c
	* @param sysDevID - системный номер устройства i2c-dev
	*/
	I2c(int realDevId, int sysDevID = 1);
	~I2c();

private:
	int realDevId;
	std::string devName;
	int fd;
};

#endif