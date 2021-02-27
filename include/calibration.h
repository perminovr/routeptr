#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <vector>
#include <mutex>
#include <string>

/*!
 * @class Calibration
 * @brief: Singleton class калибровки. Хранит точки калибровки и измеренные
 * 		значения. Позволяет получить потокобезопасный доступ к данным калибровки,
 * 		сохранить/считать калибровку с ЭНП.
 * NOTE: калибровка в ЭНП хранится в виде:
 * строка 1 - число (n) точек калибровки
 * строка 2 - точка калибровки
 * строка 3 - значение калибровки
 * ...
 * строка n*2+1
*/
class Calibration
{
public:
	~Calibration();
	static Calibration *instance();

	/*!
	* @fn Calibration::get
	* @brief: потокобезопасный метод получения данных калибровки
	* NOTE: first == last
	* @param points - точки калибровки
	* @param values - измеренные значения
	*/
	void get(std::vector <float> &points, std::vector <float> &values);

	/*!
	* @fn Calibration::set
	* @brief: потокобезопасный метод установки данных калибровки
	* NOTE: first != last
	* @param points - точки калибровки
	* @param values - измеренные значения
	*/
	bool set(const std::vector <float> &points, const std::vector <float> &values);
	bool read();
	bool save();

	/*!
	* @fn Calibration::setFileName
	* @brief: установка имени файла, в котором хранится калибровка
	* @param fileName 
	*/
	void setFileName(const std::string &fileName);

private:
	std::string fileName;
	std::vector <float> points;
	std::vector <float> values;
	std::mutex mu;

	static Calibration *self;

	Calibration() = default;
};

#endif // CALIBRATION_H