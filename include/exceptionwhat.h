#ifndef EXCEPTION_WHAT_H
#define EXCEPTION_WHAT_H

#include <string>
#include <exception>

class ExceptionWhat : public std::exception 
{
public:
	explicit ExceptionWhat(const std::string& msg) : msg(msg) {}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
private:
	std::string msg;
};

#endif // EXCEPTION_WHAT_H