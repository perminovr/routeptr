#ifndef USERCONTROL_H
#define USERCONTROL_H

#include <string>
#include <functional>
#include "stepdrv.h"
#include "ctlserver.h"

/*!
 * @class UserControl
 * @brief: Класс обработки команд пользователя
*/
class UserControl
{
public:
	/*!
	* @fn UserControl::UserControl
	* @brief:
	* @param server - обрабатывающий прием/передачу сервер
	*/
	UserControl(CtlServer *server);
	~UserControl();

	/*!
	* @fn UserControl::sendAllMoving
	* @brief: отправка всем клиентам уведомления о начале/завершении движения
	*/
	void sendAllMoving(bool start);
	/*!
	* @fn UserControl::sendAllCalibration
	* @brief: отправка всем клиентам уведомления о начале/завершении калибровки
	*/
	void sendAllCalibration(bool start);
	/*!
	* @fn UserControl::sendAllCtl
	* @brief: отправка всем клиентам уведомления о включении/отключении драйвера шагового двигателя
	*/
	void sendAllCtl(bool on);
	/*!
	* @fn UserControl::sendAllCtl
	* @brief: отправка всем клиентам уведомления о наличии ошибки дайвера шагового двигателя
	*/
	void sendAllError();

	/*!
	* @fn UserControl::setGetters
	* @brief: установка функций чтения текущей позиции и позиции назначения стрелки
	*/
	template <class Class>
	void setGetters(Class *obj, int (Class::*currPosGetter)(), int (Class::*destAzimuthGetter)()) {
		this->getCurrentPos = std::bind(currPosGetter, obj);
		this->getDestAzimuth = std::bind(destAzimuthGetter, obj);
	}

	/*!
	* @struct UserControl::Command
	* @brief: команда пользователя
	*/
	struct Command {
		enum Type {
			No,		//!< нет команды
			Set,	//!< установка на перемещение
			Calib,	//!< запуск калибровки
			Ctl		//!< влючение/отключение драйвера шагового двигателя
		} type;		//!< тип команды
		union {
			struct {
				StepDrv::Direction dir;	//!< направелние движения
				int speed;				//!< скорость вращения
				int val;				//!< точка назначения
			} set;
			struct {
				bool self;				//!< влючение/отключение
			} ctl;
		} prm; //!< параметры команды
	};
	/*!
	* @fn UserControl::setCmdHandler
	* @brief: установка обработчика команд
	*/
	template <class Class>
	void setCmdHandler(Class *obj, void (Class::*func)(const Command &)) {
		using std::placeholders::_1;
		this->cmdHandler = std::bind(func, obj, _1);
	}

protected:
	CtlServer *server;
	std::function <int()> getCurrentPos;
	std::function <int()> getDestAzimuth;
	std::function <void(const Command &)> cmdHandler;

	void onCtlReadyMsgCb(CtlClient *client);
	void sendAll(std::string msg);
};

#endif // USERCONTROL_H