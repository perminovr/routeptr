#ifndef ROUTEPRTDRV_H
#define ROUTEPRTDRV_H

#include "ctlserver.h"
#include "stepdrv.h"
#include "inifilereader.h"
#include "usercontrol.h"

/*!
 * @class RoutePrtDrv
 * @brief: Драйвер указателя
*/
class RoutePrtDrv
{
public:
	RoutePrtDrv();
	~RoutePrtDrv();

	void start();

private:
	IniFileReader *cfile;
	CtlServer *ctl;
	UserControl *uctl;
	MSensor *sensor;
	CompassDrv *compass;
	StepDrv *stepper;
	LibEvMain *libEv;

	/*!
	* @fn RoutePrtDrv::onStepperEvent
	* @brief: события от драйвера управления
	*/
	void onStepperEvent(StepDrv::Event event);
	/*!
	* @fn RoutePrtDrv::onUserCmd
	* @brief: команды пользователя
	*/
	void onUserCmd(const UserControl::Command &cmd);
};

#endif // ROUTEPRTDRV_H