#include "stepdrv.h"
#include "exceptionwhat.h"

extern void Sleep(long ms);


/*!
* @fn normalizeAzimuth
* @brief: X -> 0..360
*/
static inline void normalizeAzimuth(int &azimuth)
{
	if (abs(azimuth) > 360) {
		azimuth = azimuth - (azimuth / 360)*360;
	}
	if (azimuth < 0) {
		azimuth = 360 + azimuth;
	}
}


/*!
* @fn pulses
* @brief: число градусов до заданной точки
*/
static int pulses(int cur, int dest, int clkWise)
{
	int ret = (clkWise)? dest - cur : cur - dest;
	if (ret < 0) ret = 360 + ret;
	return ret;
}


void StepDrv::runCalibStep()
{	
	timespec_t periodPWM = {0};
	periodPWM.tv_nsec = BILLION / ((long)CALIBRATION_SPEED * this->divider * (1.0f/this->stepSize));
	this->step->runPulses(periodPWM, this->calibStep * this->divider * (1.0f/this->stepSize)); // onStepCb
}


void StepDrv::startCalibration()
{
	this->step->stopPWM();
	this->busy = true;
	this->onCalibration = true;

	this->compass->calib(false);

	calibStep = 360.0f/CALIBRATION_POINTS;
	calibPoints.clear();
	calibPoints.reserve(CALIBRATION_POINTS);
	calibPoints.push_back(0); // padding

	this->clkWise = true; 
	this->dir->setVal(this->clkWise);
	runCalibStep();
}


void StepDrv::onStepCb(void)
{
	if (this->onCalibration) {
		Sleep(CALIBRATION_PAUSE_MS);
		float azimuth = this->compass->azimuth();
		int sz = calibPoints.size();
		if (calibStep*sz >= 359.0f) {
			calibPoints[0] = azimuth;
			std::vector <float> points(sz);
			for (int i = 0; i < sz; ++i) {
				points[i] = i*this->calibStep;
			}
			this->calibration->set(points, calibPoints);
			this->calibration->save();
			this->compass->calib(true);
			this->busy = false;
			this->onCalibration = false;
			if (this->cb) {
				this->cb(Event::CalibEnd);
			}
		} else {
			calibPoints.push_back(azimuth);
			runCalibStep();
		}
	}
}


void StepDrv::move(int azimuth, Direction dir, const timespec_t &periodPWM) 
{
	if (this->onCalibration)
		return;
	if (this->busy)
		this->stop();

	this->busy = true;
	this->clkWise = true;
	
	int curpos = currentPos();
	normalizeAzimuth(azimuth);
	this->destAzimuth = azimuth;
	if (dir == Direction::Nearest) {
		int cwpulses = pulses(curpos, azimuth, true);
		int ccwpulses = pulses(curpos, azimuth, false);
		dir = (ccwpulses < cwpulses)?
				Direction::CounterClock : Direction::Clockwise;
	}
	if (dir == Direction::CounterClock)
		this->clkWise = false;

	this->dir->setVal(this->clkWise);
	timespec_t chkPeriod = {0, 5000000L}; // every 5 ms
	timerStart(this->timer, chkPeriod, StepDrv, this, onTimerCb);
	this->step->runPWM(periodPWM);
	this->mstate = MovingState::Running;
	if (this->cb)
		this->cb(Event::Started);
}


/*!
* @fn speedToPeriod
* @brief: перевод скорости град/сек в период импульсов
*/
static void speedToPeriod(int speed, int divider, float stepSize, timespec_t &period)
{
	period.tv_sec = 0;
	period.tv_nsec = 0;
	if (speed <= 1) { // 0.9 s
		period.tv_sec = 0;
		period.tv_nsec = (BILLION * 0.9f); 
	} else {
		period.tv_nsec = BILLION / ((long)speed * divider * (1.0f/stepSize));
		if (period.tv_nsec < (BILLION * 0.001f)) { // ms
			period.tv_nsec = (BILLION * 0.001f);
		}
	}
}


void StepDrv::move(int azimuth, Direction dir, int speed)
{
	timespec_t periodPWM;
	this->userSpeed = speed;
	speedToPeriod(speed, this->divider, this->stepSize, periodPWM);
	this->move(azimuth, dir, periodPWM);
}


void StepDrv::onTimerCb(Timer &timer, int revents)
{
	int cpulses = pulses(currentPos(), this->destAzimuth, this->clkWise);
	switch (this->mstate) {
		case MovingState::Running: {
			if (cpulses < 90) {
				this->mstate = MovingState::AlmostThere;
			}
		} break;
		case MovingState::AlmostThere: {	
			int slowPos = (this->userSpeed > STEPPER_SPEED_MAX)? 
					STEPPER_SPEED_MAX : this->userSpeed + STEPPER_SPEED_SLOW;
			if (cpulses <= slowPos) {
				if (this->userSpeed > STEPPER_SPEED_SLOW) {
					this->step->stopPWM();
					timespec_t periodPWM;
					speedToPeriod(STEPPER_SPEED_SLOW, this->divider, this->stepSize, periodPWM);
					this->step->runPWM(periodPWM);
				}
				this->mstate = MovingState::SlowDown;
			}
			if (cpulses <= 180) 
				break;
		} /* no break: cpulses > 180 */
		case MovingState::SlowDown: {
			if (cpulses < STEPPER_SPEED_SLOW || cpulses > 180) {
				this->mstate = MovingState::Idle;
				this->userSpeed = 0;
				this->stop();
				return;
			}
		} break;
		default: break;
	}
	timespec_t chkPeriod = {0, 5000000L}; // 5 ms
	timerStart(this->timer, chkPeriod, StepDrv, this, onTimerCb);
}


void StepDrv::onMovingStop(void)
{
	if (this->cb) {
		this->cb(Event::Stopped);
	}
}


void StepDrv::stop() 
{
	if (this->busy) {
		this->step->stopPWM();
		this->busy = false;
		this->onMovingStop();
	}
}


void StepDrv::errPauseReset(Timer &timer, int revents)
{
	this->errPause = false;
}


void StepDrv::errorHandler(ev::io &w, int revents)
{
	errEvent.end();
	if (!this->errPause) {
		this->errPause = true;
		this->cb(Event::ErrorOccured);
		timespec_t pause = {1, 0};
		timerStart(errPauseTimer, pause, StepDrv, this, errPauseReset);
	}
}


void StepDrv::control(bool enable)
{
	this->enable->setVal(enable);
	if (enable) {
		Sleep(10);
	}
}


int StepDrv::currentPos()
{
	return (int)compass->azimuth();
}


int StepDrv::destination()
{
	return destAzimuth;
}


StepDrv::StepDrv(const Init &init)
{
	if (!init.compass) {
		throw ExceptionWhat("StepDrv: compass==nullptr");
	}
	this->busy = false;
	this->onCalibration = false;
	this->clkWise = false;
	this->userSpeed = 0;
	this->mstate = MovingState::Idle;
	this->destAzimuth = 0;
	this->calibStep = 0.0;
	this->cb = nullptr;
	this->errPause = false;
	this->divider = init.divider;
	this->stepSize = init.stepSize;
	//
	this->timer = new Timer();
	this->calibration = Calibration::instance();
	this->calibration->read();
	this->compass = init.compass;
	this->compass->calib(true);
	this->step = new GpioOut(init.gpios.step);
	this->step->setCallback(this, &StepDrv::onStepCb);
	this->dir = new GpioOut(init.gpios.dir, true);
	this->enable = new GpioOut(init.gpios.en);
	this->enable->setVal(true);
	//
	if (init.gpios.err) { // error handling
		this->error = new GpioIn(init.gpios.err);
		this->errPauseTimer = new Timer();
		this->errEventWatcher.set(errEvent.getFd(), ev::READ);
		this->errEventWatcher.set<StepDrv, &StepDrv::errorHandler> (this);
		this->errEventWatcher.start();
		this->errListner = new GpioInListner(errEvent);
		this->errListner->insert(*(this->error));
	} else {
		this->error  = nullptr;
		this->errPauseTimer  = nullptr;
		this->errListner  = nullptr;
	}
}


StepDrv::~StepDrv()
{
	delete step;
	delete dir;
	delete enable;
	delete errListner;
	delete error;
	delete errPauseTimer;
	delete timer;
}