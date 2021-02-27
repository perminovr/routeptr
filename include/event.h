#ifndef EVENT_H
#define EVENT_H

#include <sys/poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

extern "C" {
	typedef struct pollfd pollfd_t;
}

/*!
* @class Event
* @brief: межпотоковые события
*/
class Event {
	static const int DSIZE = 8;
public:
	Event(int fd = eventfd(0, 0)) {
		this->fd = fd;
	}
	Event(const Event &event) {
		this->fd = event.getFd();
	}
	~Event() {
		close(this->fd);
	}
	ssize_t raise() const {
		uint8_t buf = 1;
		return write(fd, &buf, Event::DSIZE);
	}
	ssize_t end() const {
		uint8_t buf[DSIZE];
		return read(fd, &buf, Event::DSIZE);
	}
	void toPfd(pollfd_t &pfd) const {
		pfd.fd = fd;
		pfd.events = POLLIN;
	}
	int getFd() const {
		return fd;
	}
	void operator=(const Event &event) {
		this->fd = event.getFd();
	}
private:
	int fd = 0;
};

#endif