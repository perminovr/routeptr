#ifndef COMPASSDRV_H
#define COMPASSDRV_H

#include "msensor.h"
#include "calibration.h"
#include "filter.h"
#include <vector>

class LinuxThread;

/*!
 * @class CompassDrv
 * @brief: Драйвер компаса. Снимает показания датчика (акселерометр + магнитное поле) в
 * 		потоке и преобразует их в азимут. Использует соответствие осей датчика его реальному
 * 		положению; применяет фильтрацию к исходным данным. Использует калибровку к расчетному
 * 		значению.
 * NOTE: освобождает память датчика, переданного через конструктор, а также память калибровки
*/
class CompassDrv
{
public:
	/*!
	* @fn CompassDrv::CompassDrv
	* @brief
	* @param msensor - магнитный датчик
	*/
	CompassDrv(MSensor *msensor);
	~CompassDrv();

	enum Axis {
		AxisX,
		AxisY,
		AxisZ,
		nAxisX,
		nAxisY,
		nAxisZ,
	};
	/*!
	* @fn CompassDrv::mapping
	* @brief: установка соответствия осей датчика его реальному положению
	* @param msensor - магнитный датчик
	*/
	void mapping(Axis x, Axis y, Axis z);
	/*!
	* @fn CompassDrv::filtering
	* @brief: установка парамтеров фильтрации
	* @param points - число отсчетов усреднения
	* @param coef - коэффициент интегрирования
	*/
	void filtering(int points, float coef);
	/*!
	* @fn CompassDrv::calib
	* @brief: включение/выключение калибровки
	* @param enable - включение калибровки
	*/
	void calib(bool enable);

	/*!
	* @fn CompassDrv::azimuth
	* @brief: текущее расчетное направление
	*/
	float azimuth();
	
private:
	MSensor *msensor;			
	Calibration *calibration;
	volatile float m_azimuth;

	LinuxThread *thr;	//!< рабочий поток
	Axis ox, oy, oz;	//!< маппинг осей датчика
	bool m_calib;		//!< использовать калибровку

	Filter fx, fy, fz;		//!< фильтры магнитных данных
	Filter fax, fay, faz;	//!< фильтры данных акселерации

	/*!
	* @fn CompassDrv::mapAxes
	* @brief: установка соответствия
	*/
	void mapAxes(float &x, float &y, float &z);

	void loop(void);//!< функция потока
	bool lexit;		//!< флаг заверешения выполнения потока
};

#endif // COMPASSDRV_H