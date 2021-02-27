#ifndef CTLSERVER_H
#define CTLSERVER_H

#include "evwrapper.h"
#include <list>
#include <vector>
#include <functional>
#include <unistd.h>
#include <fcntl.h>


class CtlServer;

typedef std::vector<uint8_t> ByteArray;

#define setFdNonBlock(fg) {			\
	int flags = fcntl(fd, F_GETFL);	\
	flags |= O_NONBLOCK;			\
	fcntl(fd, F_SETFL, flags);		\
}

/*!
 * @class CtlClient
 * @brief: Абстракция управляющего клиента
*/
class CtlClient 
{
public:
	/*!
	* @fn CtlClient::CtlClient
	* @brief
	* @param fd - дескриптор клиента (открытый через open/accept/...)
	*/
	CtlClient(int fd) {
		this->m_deleteLater = false;
		this->fd = fd;
		setFdNonBlock(fd);
		readyReadCb = nullptr;
		w.set(fd, ev::READ);
		w.set<CtlClient, &CtlClient::watcherCb> (this);
		w.start();
	}
	virtual ~CtlClient() {
		deleteLater();
	}

	/*!
	* @fn CtlClient::getMsg
	* @brief: получение прочитанного сообщения
	*/
	ByteArray getMsg() {
		return this->storage;
	}
	/*!
	* @fn CtlClient::send
	* @brief: отправка данных клиенту
	*/
	virtual bool send(const ByteArray &msg) = 0;

protected:
	int fd; //!< дескриптор клиента

	/*!
	* @fn CtlClient::read
	* @brief: метод чтения данных клиента
	*/
	virtual ByteArray read() = 0;
	/*!
	* @fn CtlClient::deleteLater
	* @brief: помечает клиента на удаление. освобождение памяти 
	*		должен производить сервер
	*/
	void deleteLater() {
		w.stop();
		::close(fd);
		fd = -1;
		m_deleteLater = true;
	}

private:
	ev::io w;			//!< наблюдатель события на дескрипторе
	bool m_deleteLater;	//!< флаг на удаление клиента
	ByteArray storage;	//!< хранилище принятого сообщения
	std::function <void(CtlClient*)> readyReadCb; //!< callback наличия данных в хранилище

	friend class CtlServer;

	/*!
	* @fn CtlClient::watcherCb
	* @brief: callback обозревателя на событие чтения
	*/
	void watcherCb(ev::io &w, int revents) {
		if (revents & ev::ERROR) {
			return;
		}
		if (revents & ev::READ) {
			this->storage = this->read();
			if (this->readyReadCb && this->storage.size()) {
				this->readyReadCb(this);
			}
		}
	}
	/*!
	* @fn CtlClient::setReadyReadCb
	* @brief: установка callback на наличие данных в хранилище
	*/
	template <class Class>
	void setReadyReadCb(Class *obj, void (Class::*func)(CtlClient*)) {
		using std::placeholders::_1;
		this->readyReadCb = std::bind(func, obj, _1);
	}
};


/*!
 * @class CtlServer
 * @brief: Абстракция управляющего сервера
 * NOTE: сервер очищает память клиентов
*/
class CtlServer
{
public:
	CtlServer() {
		readCb = nullptr;
	}
	virtual ~CtlServer() {
		for (auto &c : clients) {
			delete c;
		}
	}

	/*!
	* @fn CtlServer::add
	* @brief: добавление клиента на обслуживание
	*/
	void add(CtlClient *client) {
		cleanUp();
		client->setReadyReadCb(this, &CtlServer::clientCb);
		clients.push_back(client);
	}
	/*!
	* @fn CtlServer::sendAll
	* @brief: отправка сообщения всем клиентам
	*/
	void sendAll(const ByteArray &msg) {
		for (auto &c : clients) {
			if (c) c->send(msg);
		}
	}

	/*!
	* @fn CtlServer::setReadyMsgCb
	* @brief: установка callback на готовность данных от клиента
	*/
	template <class Class>
	void setReadyMsgCb(Class *obj, void (Class::*func)(CtlClient*)) {
		using std::placeholders::_1;
		this->readCb = std::bind(func, obj, _1);
	}

protected:
	std::list<CtlClient*> clients;				//!< список клиентов на обслуживании
	std::function <void(CtlClient*)> readCb;	//!< callback на готовность данных от клиента

private:
	void clientCb(CtlClient *c) {
		if (readCb) readCb(c);
	}
	/*!
	* @fn CtlServer::cleanUp
	* @brief: очистка списка клиетнов от помеченных на удаление
	*/
	void cleanUp() {
		CtlClient *prev = nullptr;
		for (auto &c : clients) {
			if (prev) {
				clients.remove(prev);
				delete prev;
				prev = nullptr;
			}
			if (c && c->m_deleteLater) {
				prev = c;
			}
		}
		if (prev) {
			clients.remove(prev);
			delete prev;
			prev = nullptr;
		}
	}
};

#endif // CTLSERVER_H