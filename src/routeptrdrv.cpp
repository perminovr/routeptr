#include "routeptrdrv.h"
#include "lsm303dlhc.h"
#include "qmc5883l.h"
#include "hmc5883.h"
#include "tcpctlserver.h"
#include "exceptionwhat.h"
#include <iostream>

#define CONFIG_FILE_NAME "config.ini"

using namespace std;


void RoutePrtDrv::onUserCmd(const UserControl::Command &cmd)
{
	switch (cmd.type) {
		case UserControl::Command::Type::Set: {
			stepper->move(cmd.prm.set.val, cmd.prm.set.dir, cmd.prm.set.speed);
		} break;
		case UserControl::Command::Type::Calib: {
			stepper->startCalibration();
			uctl->sendAllCalibration(true);
		} break;
		case UserControl::Command::Type::Ctl: {
			stepper->control(cmd.prm.ctl.self);
			uctl->sendAllCtl(cmd.prm.ctl.self);
		} break;
		default: /* NOP */ break;
	}
}


void RoutePrtDrv::onStepperEvent(StepDrv::Event event)
{
	switch (event) {
		case StepDrv::Event::Started: {
			uctl->sendAllMoving(true);
		} break;
		case StepDrv::Event::Stopped: {
			uctl->sendAllMoving(false);
		} break;
		case StepDrv::Event::CalibEnd: {
			uctl->sendAllCalibration(false);
		} break;
		case StepDrv::Event::ErrorOccured: {
			uctl->sendAllError();
		} break;
	}
}


void RoutePrtDrv::start()
{
	try {
		libEv->loop();
	} catch (exception &ex) {
		cerr << ex.what() << endl;
	}
}


#define readMSensorAxis(a) \
	CompassDrv::Axis a;											\
	{															\
		string axisstr = conf["msensor"]["axis_" #a];	 		\
		if (axisstr == "axis_x") a = CompassDrv::AxisX;			\
		else if (axisstr == "axis_y") a = CompassDrv::AxisY;	\
		else if (axisstr == "axis_z") a = CompassDrv::AxisZ;	\
		else if (axisstr == "-axis_x") a = CompassDrv::nAxisX;	\
		else if (axisstr == "-axis_y") a = CompassDrv::nAxisY;	\
		else if (axisstr == "-axis_z") a = CompassDrv::nAxisZ;	\
		else throw exception();									\
	}


RoutePrtDrv::RoutePrtDrv()
{
	cfile = new IniFileReader(CONFIG_FILE_NAME);
	cfile->parse();
	auto conf = cfile->getConfig();

	// common
	{
		string fn = conf["common"]["calibfile"];
		if (!fn.length()) {
			throw ExceptionWhat("Couldn't get [common]calibfile prop");
		}
		Calibration *calib = Calibration::instance();
		calib->setFileName(fn);
	}

	// server
	try {
		string ip = conf["tcpctl"]["ip"];
		int port = stoi(conf["tcpctl"]["port"]);
		int ka = stoi(conf["tcpctl"]["keepalive"]);
		ctl = new TcpCtlServer(ip, (uint16_t)port, (bool)ka);
	} catch (exception &ex) {
		cerr << ex.what() << endl;
		throw ExceptionWhat("Couldn't parse [tcpctl] props");
	}

	// magnetic sensor
	try {
		string sensorName = conf["msensor"]["device"];
		int i2cbus = stoi(conf["msensor"]["i2cbus"]);
		if (sensorName == "lsm") {
			sensor = new Lsm303dlhc(i2cbus);
		} else if (sensorName == "hmc") {
			sensor = new Hmc5883(i2cbus);
		} else {
			sensor = new Qmc5883l(i2cbus);
		}
		
		compass = new CompassDrv(sensor);
		readMSensorAxis(x);
		readMSensorAxis(y);
		readMSensorAxis(z);
		compass->mapping(x, y, z);
		int points = stoi(conf["msensor"]["fpoints"]);
		float coef = stof(conf["msensor"]["fcoef"]);
		compass->filtering(points, coef);
	} catch (exception &ex) {
		cerr << ex.what() << endl;
		throw ExceptionWhat("Couldn't parse [msensor] props");
	}
	
	// step drv
	try {
		int en, dir, step, err, div;
		float stepsize;
		en = stoi(conf["stepper"]["en"]);
		dir = stoi(conf["stepper"]["dir"]);
		step = stoi(conf["stepper"]["step"]);
		err = stoi(conf["stepper"]["err"]);
		div = stoi(conf["stepper"]["divider"] );
		stepsize = stof(conf["stepper"]["stepsize"] );
		StepDrv::Init init = {
			.compass = compass,
			.gpios = {
				.en = en,
				.dir = dir,
				.step = step,
				.err = err
			},
			.divider = div,
			.stepSize = stepsize
		};
		stepper = new StepDrv(init);
		stepper->setCallback(this, &RoutePrtDrv::onStepperEvent);
	} catch (exception &ex) {
		cerr << ex.what() << endl;
		throw ExceptionWhat("Couldn't parse [stepper] props");
	}

	// user command ctl
	{
		uctl = new UserControl(ctl);
		uctl->setGetters(stepper, &StepDrv::currentPos, &StepDrv::destination);
		uctl->setCmdHandler(this, &RoutePrtDrv::onUserCmd);
	}

	libEv = LibEvMain::instance();

	delete cfile;
}


RoutePrtDrv::~RoutePrtDrv()
{
	delete libEv;
	delete stepper;
	delete compass;
	delete uctl;
	delete ctl;
}

