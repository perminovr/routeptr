#ifndef TCPCTLSERVER_H
#define TCPCTLSERVER_H

#include "ctlserver.h"

/*!
 * @class TcpCtlClient
 * @brief: Управляющий Tcp клиент
*/
class TcpCtlClient : public CtlClient
{
public:
	TcpCtlClient(int fd, bool keepalive);
	virtual ~TcpCtlClient() = default;

	virtual bool send(const ByteArray &msg) override;

protected:
	virtual ByteArray read() override;
};

/*!
 * @class TcpCtlClient
 * @brief: Управляющий Tcp сервер
*/
class TcpCtlServer : public CtlServer
{
public:
	/*!
	* @fn TcpCtlServer::TcpCtlServer
	* @brief: 
	* @param ip
	* @param port
	* @param keepalive - включение keepalive для клиентов (5:2:3)
	*/
	TcpCtlServer(const std::string &ip, uint16_t port, bool keepalive);
	~TcpCtlServer();

private:
	int fd;			//!< дескриптор для accept
	ev::io w;		//!< наблюдатель для accept
	bool keepalive;

	/*!
	* @fn TcpCtlServer::acceptCb
	* @brief: callback на подключение клиентов
	*/
	void acceptCb(ev::io &w, int revents);
};

#endif // TCPCTLSERVER_H