#ifndef EVWRAPPER_H
#define EVWRAPPER_H

#define EV_USE_FLOOR 1
#define EV_USE_MONOTONIC 1
#define EV_USE_NANOSLEEP 1

#define EV_STANDALONE 1
#define EV_USE_POLL 1
#define EV_USE_EPOLL 0
#define EV_USE_SELECT 0
#define EV_USE_IOURING 0
#define EV_MULTIPLICITY 0
#define EV_USE_EVENTFD 1
#include "ev++.h"

extern "C" {
	typedef struct timespec timespec_t;
}

#define BILLION 	1000000000L	
extern double toDouble(const timespec_t &ts);
extern void operator*=(timespec_t &lhs, float f);

typedef ev::timer Timer;

#define timerStart(timer, ts, class, self, cb) \
	timer->set<class, &class::cb> ( ((class*)self) ); \
	timer->set(toDouble(ts)); \
	timer->start();

/*!
 * @class LibEvMain_p
 * @brief: private часть LibEvMain
*/
class LibEvMain_p;

/*!
 * @class LibEvMain
 * @brief: singleton обертка libev. Позволяет запускать main_loop libev
 * 		в потоке, ставить его на паузу. Позволяет запускать main_loop
 * 		в текущем потоке @ref LibEvMain::loop
 * NOTE: http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod
*/
class LibEvMain
{
public:
	~LibEvMain();
	static LibEvMain *instance();

	/*!
	* @fn LibEvMain::start
	* @brief: запуск main_loop libev в потоке
	*/
	void start();

	/*!
	* @fn LibEvMain::pause
	* @brief: установка main_loop libev в потоке на паузу
	*/
	void pause();
	/*!
	* @fn LibEvMain::resume
	* @brief: возобновление работы main_loop libev в потоке
	*/
	void resume();

	/*!
	* @fn LibEvMain::loop
	* @brief: запуск main_loop libev в текущем потоке
	*/
	void loop();

private:
	LibEvMain();

	static LibEvMain *self;

	LibEvMain_p *priv;
	friend class LibEvMain_p;
};

#endif // EVWRAPPER_H
