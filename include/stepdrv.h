#ifndef STEPDRV_H
#define STEPDRV_H

#include "compassdrv.h"
#include "gpiodrv.h"

#define CALIBRATION_POINTS		8
#define CALIBRATION_SPEED		7
#define CALIBRATION_PAUSE_MS	2500

#define STEPPER_SPEED_MAX		50
#define STEPPER_SPEED_SLOW		5


/*!
 * @class StepDrv
 * @brief: драйвер управления шаговым двигателем
 * 		на основе сигналов enable, dir, step
*/
class StepDrv {
public:
	/*!
	* @struct StepDrv::Init
	* @brief: структура инициализации драйвера
	*/
	struct Init {
		CompassDrv *compass;	//!< указатель на компас
		struct {
			int en;				//!< номер ножки включения драйвера шагового двигателя
			int dir;			//!< номер ножки направления шагового двигателя
			int step;			//!< номер ножки шага двигателя
			int err;			//!< номер ножки сигнала ошибки драйвера шагового двигателя
		} gpios;
		int divider;			//!< установленный делитель на драйвере шагового двигателя
		float stepSize;			//!< размер шага в град
	};
	StepDrv(const Init &init);
	~StepDrv();

	/*!
	* @enum StepDrv::Event
	* @brief: события драйвера @ref StepDrv::setCallback
	*/
	enum Event {
		Started,		//!< начало движения @ref StepDrv::move
		Stopped,		//!< останов движения
		CalibEnd,		//!< завершение процедуры калибровки
		ErrorOccured	//!< возникновение ошибки
	};

	/*!
	* @fn StepDrv::currentPos
	* @brief: получение текущей позиции стрелки
	*/
	int currentPos();
	/*!
	* @fn StepDrv::destination
	* @brief: получение заданной позиции назначения
	*/
	int destination();

	/*!
	* @enum StepDrv::Direction
	* @brief: направление перемещения
	*/
	enum Direction {
		CounterClock,
		Clockwise,
		Nearest
	};
	/*!
	* @fn StepDrv::move
	* @brief: запуск перемещения
	* @param azimuth - точка назначения
	* @param dir - направление
	* @param speed - скорость вращения в град/сек
	*/
	void move(int azimuth, Direction dir, int speed);
	void stop();

	/*!
	* @fn StepDrv::control
	* @brief: включение/отключение драйвера шагового двигателя
	*/
	void control(bool enable);

	/*!
	* @fn StepDrv::startCalibration
	* @brief: запуск процедуры калибровки
	*/
	void startCalibration();

	/*!
	* @fn StepDrv::setCallback
	* @brief: установка callback для получения событий с драйвера
	*/
	template <class Class>
	void setCallback(Class *obj, void (Class::*func)(Event)) {
		using std::placeholders::_1;
		this->cb = std::bind(func, obj, _1);
	}

private:
	Timer *timer;				//!< таймер опроса текущей позиции во время движения
	CompassDrv *compass;		//!< направляющий компас
	Calibration *calibration;	//!< данные калибровки
	GpioOut *step;				//!< 
	GpioOut *dir;				//!< 
	GpioOut *enable;			//!< 
	GpioIn *error;				//!< 
	float stepSize;				//!< размер шага шагового двигателя
	int divider;				//!< делитель драйвера шагового двигателя
	bool busy;					//!< флаг работы драйвера
	bool onCalibration;			//!< флаг драйвера в состоянии калибровки
	bool clkWise;				//!< направление движение по часовой стрелке
	int destAzimuth;			//!< точка назначения
	int userSpeed;				//!< заданная пользователем скорость

	/*!
	* @enum StepDrv::MovingState
	* @brief: состояние перемещения
	*/
	enum MovingState {
		Idle,
		Running,
		AlmostThere,
		SlowDown
	} mstate;

	float calibStep;			//!< шаг калибровки в градусах
	std::vector <float> calibPoints; //!< измерения в точках калибровки

	Timer *errPauseTimer;		//!< таймер для паузы передачи сообщения об ошибке
	GpioInListner *errListner;	//!< слушатель ошибки на пине err
	::Event errEvent;			//!< событие на пине err
	ev::io errEventWatcher;		//!< наблюдатель за событием на пине err
	bool errPause;				//!< флаг паузы на передачу сообения об ошибке

	std::function <void(Event)> cb;	//!< callback для событий драйвера

	/*!
	* @fn StepDrv::onTimerCb
	* @brief: callback на таймере опроса текущей позиции во время движения
	* NOTE: выполняет автомат движения: ближе к точке назначения скорость снижается
	*/
	void onTimerCb(Timer &timer, int revents);
	/*!
	* @fn StepDrv::onStepCb
	* @brief: callback на завершении выдачи серии импульсов с пина step
	* NOTE: работает в калибровке
	*/
	void onStepCb(void);
	/*!
	* @fn StepDrv::runCalibStep
	* @brief: запуск шага калибровки
	*/
	void runCalibStep();
	void move(int azimuth, Direction dir, const timespec_t &periodPWM);
	void onMovingStop(void);
	void errorHandler(ev::io &w, int revents);
	void errPauseReset(Timer &timer, int revents);
};

#endif /* STEPDRV_H */