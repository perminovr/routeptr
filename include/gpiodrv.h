#ifndef GPIODRV_H
#define GPIODRV_H

#include "event.h"
#include "linuxthread.h"
#include "evwrapper.h"

#include <string>
#include <mutex>
#include <list>
#include <functional>
#include <sys/epoll.h>


/*!
 * @class Gpio
 * @brief: базовый класс gpio. Предназначен для экспорта
 * 		и определения направления IN/OUT
*/
class Gpio {
public:
	std::string num; 	//!< номер gpio linux
	std::string value; 	//!< имя файла для чтения/записи значения
	bool inverse;	//!< инвертирование сигнала как на запись, так и на чтение
	/*!
	* @fn Gpio::getGpioNum
	* @brief: перевод порта и номера ножки в значение gpio linux
	*/
	static int getGpioNum(int port, int pin);
	std::string getFullValueName() const;
	enum Direction {
		GpioInput,
		GpioOutput,
	};
	Gpio(int num, Direction dir, bool inverse = false);

protected:
	bool _isExported() const;
	void _export() const;
	void _unexport() const;
	void _direction(Direction dir) const;
};


/*!
 * @class GpioOut
 * @brief:
*/
class GpioOut : public Gpio {
public:	
	GpioOut(int num, bool inverse = false);
	~GpioOut();
	/*!
	* @fn GpioOut::runPWM
	* @brief: запуск ШИМ. ШИМ должен быть выключен, установка значения
	* 		сигнала с помощью @ref GpioOut::setOnTime должно быть завершено
	*		@ref GpioOut::isBusy
	* @param periodPWM - период ШИМ
	* @param dutyCycle - скважность
	*/
	bool runPWM(const timespec_t &periodPWM, int dutyCycle = 50);
	/*!
	* @fn GpioOut::runPulses
	* @brief: запуск выдачи серии импульсов. условия запуска аналогичны
	* 		@ref GpioOut::runPWM
	* @param periodPWM - период ШИМ
	* @param pulseCnt - число импульсов (одного фронта)
	*/
	bool runPulses(const timespec_t &periodPWM, int pulseCnt);
	/*!
	* @fn GpioOut::setOnTime
	* @brief: установить сигнал на заданное время. условия запуска аналогичны
	* 		@ref GpioOut::runPWM
	*/
	bool setOnTime(bool val, const timespec_t &time);
	/*!
	* @fn GpioOut::stopPWM
	* @brief: останов ШИМ
	*/
	void stopPWM();
	/*!
	* @fn GpioOut::stopPulses
	* @brief: останов выдачи импульсов
	*/
	void stopPulses();
	/*!
	* @fn GpioOut::setOnTime
	* @brief: установить сигнал
	*/
	bool setVal(bool val);
	/*!
	* @fn GpioOut::setCallback
	* @brief: callback для уведомлении о завершении выдачи импульсов
	* @param obj
	* @param func
	*/
	template <class Class>
	void setCallback(Class *obj, void (Class::*func)()) {
		this->pulsesEnd = std::bind(func, obj);
	}
	bool isBusy() const;

protected:
	Timer *timer;	//!< таймер для отработки ШИМ
	bool prevVal;	//!< сохранение значения до изменения
	bool busy;		//!< сигнал занятости драйвера
	timespec_t dutyTime;	//!< скважность
	timespec_t pauseTime;	//!< простой
	int pulseCnt;			//!< оставшееся число импульсов на выдачу
	std::function <void()> pulsesEnd;	//!< callback для уведомлении о завершении выдачи импульсов

	/*!
	* @fn GpioOut::setOnTimeCb
	* @brief: callback для снятия сигнала по таймауту
	*/
	void runPWMCb(Timer &, int );
	/*!
	* @fn GpioOut::runPWMCb
	* @brief: callback ШИМ для переключения уровня
	*/
	void setOnTimeCb(Timer &, int );
};


class GpioInListner;


/*!
 * @class GpioOut
 * @brief:
*/
class GpioIn : public Gpio {
public:
	/*!
	* @fn GpioIn::getVal
	* @brief: получение значения сигнала
	*/
	bool getVal(bool &val) const;
	GpioIn(int num, bool inverse = false);
	~GpioIn() = default;

protected:
	friend class GpioInListner;
};


/*!
 * @class GpioInListner
 * @brief: класс для получения событий на gpio
*/
class GpioInListner {
public:
	/*!
	* @fn GpioInListner::GpioInListner
	* @brief:
	* @param event - сигнал фиксации события
	*/
	GpioInListner(Event &event);
	~GpioInListner();

	/*!
	* @fn GpioInListner::insert
	* @brief: вставка gpio на ожидание события (фронта)
	*/
	bool insert(GpioIn &gpio);
	/*!
	* @fn GpioInListner::remove
	* @brief: удаление gpio с ожидания
	*/
	bool remove(const GpioIn &gpio);
	/*!
	* @fn GpioInListner::getEventsList
	* @brief: получение списка gpio, на которых были зафиксированы события
	*/
	std::list<GpioIn *> getEventsList();

protected:
	/*!
	* @struct GpioCtl
	* @brief: объект gpio: имя, дескриптор, указатель на объект @ref GpioIn
	*/
	struct GpioCtl {
		std::string name;
		int fd;
		GpioIn *self;
		inline bool operator==(const GpioCtl &g) { return name == g.name; }
	};
	int epfd;			//!< дескриптор epoll
	LinuxThread *thr;	//!< основной поток
	Event evKill;		//!< событие останова потока
	Event evUpdate;		//!< событие обновления GpioInListner::gpios
	std::mutex mu;		//!< мьютекс обновления GpioInListner::gpios
	std::list<GpioCtl> gpios;	//!< список gpio на ожидании событий
	std::list<GpioIn *> evgpios;//!< список gpio, на которых были зафиксированы события
	Event event;
	/*!
	* @fn GpioInListner::listen
	* @brief: основная функция прослушивания событий на gpio
	*/
	void listen();
	/*!
	* @fn GpioInListner::addToEpoll
	* @brief: добавление события в epoll
	*/
	void addToEpoll(int fd, uint32_t events = EPOLLIN);
	/*!
	* @fn GpioInListner::delFromEpoll
	* @brief: удалние события из epoll
	*/
	void delFromEpoll(int fd, uint32_t events = EPOLLIN);
};

#endif // GPIODRV_H