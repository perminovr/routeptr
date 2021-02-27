#include "tcpctlserver.h"
#include "exceptionwhat.h"

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

extern "C" {
	typedef struct sockaddr sockaddr_t;
	typedef struct sockaddr_in sockaddr_in_t;
}


bool TcpCtlClient::send(const ByteArray &msg)
{
	int sz = msg.size();
	int res = ::send(this->fd, &(msg[0]), sz, 0);
	return (res == sz);
}


ByteArray TcpCtlClient::read()
{
	ByteArray ret;
	uint8_t buf[1024];
	int res = ::recv(this->fd, buf, 1024, 0);
	if (res <= 0) {
		this->deleteLater();
	} else {
		ret.insert(ret.end(), &(buf[0]), &(buf[res]));
	}
	return ret;
}


TcpCtlClient::TcpCtlClient(int fd, bool keepalive) : CtlClient(fd)
{
	int val;
	if (keepalive) {
		val = 1;
		::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
		val = 5;
		::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val));
		val = 2;
		::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val));
		val = 3;
		::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val));
	}
	struct linger lin = {
		.l_onoff = 1,
		.l_linger = 0
	};
	::setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(struct linger));
}


void TcpCtlServer::acceptCb(ev::io &w, int revents)
{
	if (revents & ev::ERROR) {
		return;
	}
	if (revents & ev::READ) {
		sockaddr_t addr;
		socklen_t addrlen = sizeof(sockaddr_t);
		int cfd = ::accept(this->fd, &addr, &addrlen);
		if (cfd > 0) {
			TcpCtlClient *client = new TcpCtlClient(cfd, this->keepalive);
			this->add(client);
		}
	}
}


TcpCtlServer::TcpCtlServer(const std::string &ip, uint16_t port, bool keepalive) : CtlServer()
{
	this->keepalive = keepalive;
	fd = ::socket(AF_INET, SOCK_STREAM, 0);
	setFdNonBlock(fd);

	// binding
	{
   		sockaddr_in_t addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
		if (::bind(fd, (sockaddr_t*)&addr, sizeof(sockaddr_in_t)) < 0) {
			throw ExceptionWhat("Couldn't bind [" + ip + ":" + std::to_string(port) + "]: " + ::strerror(errno));
		}
	}

	if (::listen(fd, SOMAXCONN) < 0) {
		throw ExceptionWhat(std::string("Couldn't start to listen: ") + ::strerror(errno));
	}
	
	w.set(fd, ev::READ);
	w.set<TcpCtlServer, &TcpCtlServer::acceptCb> (this);
	w.start();
}


TcpCtlServer::~TcpCtlServer()
{
	w.stop();
	::close(fd);
}

