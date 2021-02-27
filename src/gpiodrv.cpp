#include "gpiodrv.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include "exceptionwhat.h"

extern "C" {
	typedef struct stat stat_t;
	typedef struct epoll_event epoll_event_t;
}

using namespace std;


int Gpio::getGpioNum(int port, int pin) 
{
	return port*32 + pin;
}


std::string Gpio::getFullValueName() const 
{
	return this->value;
}


Gpio::Gpio(int num, Direction dir, bool inverse) 
{
	this->inverse = inverse;
	this->num = to_string(num);
	this->value = "/sys/class/gpio/gpio" + this->num + "/value";
	if ( !_isExported() )
		_export();
	_direction(dir);
}


bool Gpio::_isExported() const 
{
	stat_t status;
	return (stat(value.c_str(), &status) == 0)? true : false;
}


void Gpio::_export() const 
{
	FILE *exp = fopen("/sys/class/gpio/export", "w");
	(void)fwrite(this->num.c_str(), this->num.size(), 1, exp);
	fclose(exp);
}


void Gpio::_unexport() const 
{
	FILE *exp = fopen("/sys/class/gpio/unexport", "w");
	(void)fwrite(this->num.c_str(), this->num.size(), 1, exp);
	fclose(exp);
}


void Gpio::_direction(Direction dir) const 
{
	std::string file = "/sys/class/gpio/gpio" + this->num + "/direction";
	FILE *fdir = fopen(file.c_str(), "w");
	const char *sdir = (dir == Direction::GpioInput)? "in" : "out";
	(void)fwrite(sdir, strlen(sdir), 1, fdir);
	fclose(fdir);
}


bool GpioOut::runPWM(const timespec_t &periodPWM, int dutyCycle) 
{
	if (!this->busy && dutyCycle <= 100 && dutyCycle >= 0) {
		this->pulseCnt = -1; // сброс счетчика, т.к. разделяемый callback обработчик
		this->busy = true;
		this->timer = timer;
		{
			float dc = ((float)dutyCycle / 100.0f);
			this->dutyTime = periodPWM;
			this->pauseTime = periodPWM;
			if (this->inverse) {
				this->dutyTime *= (1.0f - dc);
				this->pauseTime *= dc;
			} else {
				this->dutyTime *= dc;
				this->pauseTime *= (1.0f - dc);
			}
		}
		// первое и последующие переключения по callback
		timerStart(timer, this->dutyTime, GpioOut, this, runPWMCb);
		return true;
	}
	return false;
}


void GpioOut::stopPWM()
{
	if (this->busy) {
		this->busy = false;
		this->timer->stop();
	}
}


bool GpioOut::runPulses(const timespec_t &periodPWM, int pulseCnt) 
{
	bool ret = runPWM(periodPWM);
	this->pulseCnt = pulseCnt*2; // установка числа импульсов после запуска, сработает раньше чем callback
	return ret;
}


void GpioOut::stopPulses() 
{
	stopPWM();
}


bool GpioOut::setOnTime(bool val, const timespec_t &time)
{
	if (!this->busy) {
		setVal(val);
		this->busy = true;
		// сигнал будет снят по callback
		timerStart(timer, time, GpioOut, this, setOnTimeCb);
		return true;
	}
	return false;
}


bool GpioOut::setVal(bool val) 
{
	this->prevVal = val;
	if (this->inverse) 
		val = !val;
	int fd = open(value.c_str(), O_WRONLY);
	int res = write(fd, (val)? "1" : "0", 1);
	close(fd);
	return (res == 1)? true : false;
}


void GpioOut::runPWMCb(Timer &w, int revents)
{
	if (this->busy) {			
		if (this->pulseCnt != -1) { // счетчик импульсов
			this->pulseCnt--;
			if (this->pulseCnt == 0) {
				this->pulseCnt = -1;
				this->busy = false;
				if (this->pulsesEnd)
					this->pulsesEnd();
				return;
			}
		}
		timespec_t time = (this->prevVal)? 
			this->pauseTime : this->dutyTime;
		this->setVal(!this->prevVal);
		timerStart(this->timer, time, GpioOut, this, runPWMCb);
	}
}


void GpioOut::setOnTimeCb(Timer &w, int revents)
{
	this->setVal(!this->prevVal);
	this->busy = false;
}


bool GpioOut::isBusy() const 
{
	return this->busy;
}


GpioOut::GpioOut(int num, bool inverse) : Gpio(num, Gpio::GpioOutput, inverse) 
{
	this->timer = new Timer();
	this->prevVal = false;
	this->busy = false;
}


GpioOut::~GpioOut()
{
	stopPWM();
	delete timer;
}





bool GpioIn::getVal(bool &val) const 
{
	int fd = open(value.c_str(), O_RDONLY);
	char sval;
	int res = read(fd, &sval, 1);
	close(fd);
	if (res == 1) {
		val = (sval == '0') ^ this->inverse;
		return true;
	}
	return false;
}


GpioIn::GpioIn(int num, bool inverse) : Gpio(num, Gpio::GpioInput, inverse) 
{}


static void setRisingEdge(string num)
{
	string fn = "/sys/class/gpio/gpio" + num + "/edge";
	FILE *f = fopen(fn.c_str(), "w");
	(void)fwrite("rising", 6, 1, f);
	fclose(f);
}


bool GpioInListner::insert(GpioIn &gpio) 
{
	GpioCtl ctl = {gpio.getFullValueName(), 0};
	// add if no such gpio
	auto it = find(gpios.begin(), gpios.end(), ctl);		
	if (it == gpios.end()) {
		// set interrupt edge
		setRisingEdge(gpio.num);
		// open fd for epoll
		ctl.fd = open(ctl.name.c_str(), O_RDONLY);
		ctl.self = &gpio; // save for callback
		mu.lock();
		gpios.push_back(ctl);
		addToEpoll(ctl.fd, EPOLLET); // unblock epoll
		mu.unlock();
		evUpdate.raise();
		return true;
	}
	return false;
}


bool GpioInListner::remove(const GpioIn &gpio) 
{
	GpioCtl ctl = {gpio.getFullValueName(), 0};
	// del if exist
	auto it = find(gpios.begin(), gpios.end(), ctl);
	if (it != gpios.end()) {
		mu.lock();
		gpios.remove(ctl);
		delFromEpoll(ctl.fd, EPOLLET); // unblock epoll
		mu.unlock();
		evUpdate.raise();
		return true;
	}
	return false;
}


std::list<GpioIn *> GpioInListner::getEventsList()
{
	mu.lock();
	std::list<GpioIn *> ret = this->evgpios;
	this->evgpios.clear();
	mu.unlock();
	return ret;
}


GpioInListner::GpioInListner(Event &event) 
{
	epfd = epoll_create(1);
	addToEpoll(evUpdate.getFd());
	addToEpoll(evKill.getFd());
	this->event = event;
	thr = new LinuxThread(this, &GpioInListner::listen);
}


GpioInListner::~GpioInListner() 
{
	evKill.raise();
	thr->join();
	delete thr;
	delFromEpoll(evUpdate.getFd());
	delFromEpoll(evKill.getFd());
}


void GpioInListner::listen() 
{
	epoll_event_t events;
	for (;;) {
		epoll_wait(this->epfd, &events, 1, -1);	// ожидание событий без таймаута
		if (events.events) {
			if (events.data.fd == this->evUpdate.getFd()) {				
				this->evUpdate.end();
			} else if (events.data.fd == this->evKill.getFd()) {
				LinuxThread::exit();
			} else {
				this->mu.lock();
				for (auto &g : this->gpios) {
					if (events.data.fd == g.fd) {
						auto p = find(this->evgpios.begin(), this->evgpios.end(), g.self);
						if (p == this->evgpios.end()) {
							this->evgpios.push_back(g.self);
						}
						this->event.raise();
					}
				}
				this->mu.unlock();
			}
		}	
	}
}


void GpioInListner::addToEpoll(int fd, uint32_t events) 
{
	epoll_event_t ev;
	ev.data.fd = fd;
	ev.events = events;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}


void GpioInListner::delFromEpoll(int fd, uint32_t events) 
{
	epoll_event_t ev = {0};
	ev.data.fd = fd;
	ev.events = events;
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
}