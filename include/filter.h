#ifndef FILTER_H
#define FILTER_H

#include <vector>

/*!
 * @class Filter
 * @brief: фильтр сигнала
*/
class Filter
{
public:
	Filter();
	~Filter() = default;

	/*!
	* @fn Filter::init
	* @brief: инициализация параметров фильтрации
	* @param minpoints - минимальное число точек для усреднения
	* @param k - коэффициент интегрирования (k <= 0.001 - интегрирование отключено)
	*/
	void init(int minpoints, float k);

	/*!
	* @fn Filter::push
	* @brief: установка очередного значения на усреднение
	*/
	void push(float v);
	/*!
	* @fn Filter::isReady
	* @brief: минимальное число точек для усреднения задано
	*/
	bool isReady() const;
	/*!
	* @fn Filter::value
	* @brief: получение фильтрованного значения. если минимальное число точек
	*		не было получено, то будет возвращено предыдущее значение
	*/
	float value();

	/*!
	* @fn Filter::reset
	* @brief: сброс усреденения (интегрирование не сбрасывается)
	*/
	void reset();

private:
	int points;	//!< минимальное число точек для усреднения
	float k;	//!< коэффициент интегрирования (k <= 0.001 - интегрирование отключено)
	std::vector <float> values; //!< значения для усреденения
	float val;	//!< фильтрованное значения
	bool ready;	//!< готовность к выдаче значения
};

#endif // FILTER_H