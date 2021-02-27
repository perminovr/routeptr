#include "compassdrv.h"
#include "linuxthread.h"
#include "exceptionwhat.h"
#include <cmath>
#include <iomanip>
#include <unistd.h>
#include <iostream>

extern void Sleep(long ms);

#define READ_PERIOD_MS	10

using namespace std;

/*!
 * @fn range_map
 * @brief: отображение точки из входного пространства (диапазона) в выходное
*/
template <typename T>
T range_map(T x, T in_min, T in_max, T out_min, T out_max)
{
	return (x - in_min) * (T)((double)(out_max - out_min) / (double)(in_max - in_min)) + out_min;
}

/*!
 * @fn range_map
 * @brief: отображение точки из входного пространства (диапазона) в выходное с
 * 		предварительной корректировкой 
*/
template <typename T>
bool range_map(T x, T in_min, T in_max, T out_min, T out_max, T &ret)
{
	if (in_max < in_min) { // e.g 340...10
		if (0 < x && x < in_max) // e.g. 340..(x=2)..10
			x += (T)360;
		in_max += (T)360;
	} 
	if (out_max < out_min) out_max += (T)360;
	if (x < in_min || x > in_max) return false; // next range
	ret = range_map(x, in_min, in_max, out_min, out_max);
	if (ret >= (T)360) ret = (T)0;
	return true;
}


#define mapAxis(a) a = \
	(o##a == AxisX)? tx :\
	(o##a == AxisY)? ty :\
	(o##a == AxisZ)? tz :\
	(o##a == nAxisX)? -tx :\
	(o##a == nAxisY)? -ty :\
	-tz;

void CompassDrv::mapAxes(float &x, float &y, float &z)
{
	float tx = x;
	float ty = y;
	float tz = z;
	mapAxis(x);
	mapAxis(y);
	mapAxis(z);
}


void CompassDrv::loop(void)
{
	float x, y, z;
	float ax, ay, az;
	double fNorm;
	double fSinRoll, fSinPitch;
	double fCosRoll, fCosPitch;
	// double RollAng, PitchAng;
	double fTiltedX, fTiltedY;
	double fTiltedZ;
	double HeadingValue;

	vector <float> cpoints;
	vector <float> cvalues;
	bool calibTrigger = true;

	for (;!lexit;) {
		this->msensor->getMagn(x, y, z);
		this->msensor->getAcc(ax, ay, az);
		this->mapAxes(x, y, z);
		this->mapAxes(ax, ay, az);

		// filtering
		fx.push(x); fy.push(y); fz.push(z);
		fax.push(ax); fay.push(ay); faz.push(az);
		if ( !fx.isReady() ) { // one of
			Sleep(READ_PERIOD_MS);
			continue;
		}

		#ifdef RDEBUG
		{
			system("clear");
			cout << "CompassDrv::loop ================================" << endl;
			cout << "x: " << x << " y: " << y << " z: " << z << endl;
			cout << "ax: " << ax << " ay: " << ay << " az: " << az << endl;
		}
		#endif

		// get filtering values
		x = fx.value(); y = fy.value(); z = fz.value();
		ax = fax.value(); ay = fay.value(); az = faz.value();
		
		// compute azimuth
		fNorm = sqrt(ax*ax + ay*ay + az*az);
		fSinRoll = -(double)ay/fNorm;
		fSinPitch = (double)ax/fNorm;
		fCosRoll = sqrt(1.0 - fSinRoll*fSinRoll);
		fCosPitch = sqrt(1.0 - fSinPitch*fSinPitch);
		fTiltedX = x*fCosPitch + z*fSinPitch;
		fTiltedY = x*fSinRoll*fSinPitch + y*fCosRoll - z*fSinRoll*fCosPitch;
		fTiltedZ = -x*fCosRoll*fSinPitch + y*fSinRoll + z*fCosRoll*fCosPitch;
		// RollAng = acos(fCosRoll) * 180.0/M_PI;
		// PitchAng = acos(fCosPitch) * 180.0/M_PI;
		HeadingValue = atan2(fTiltedY, fTiltedX) * 180.0/M_PI;
		if (HeadingValue < 0.0)
			HeadingValue = 360 + HeadingValue;

		float heading = (float)HeadingValue;
		// apply calibration
		if (m_calib) {
			// read on trigger
			if (calibTrigger) {
				calibTrigger = false;
				calibration->get(cpoints, cvalues);
			}
			float istart;
			float iend;
			float ostart;
			float oend;
			int sz = cpoints.size();
			for (int i = 0; i < sz-1; ++i) {
				istart = cvalues[i+0];
				iend = cvalues[i+1];
				ostart = cpoints[i+0];
				oend = cpoints[i+1];
				if (range_map(heading, istart, iend, ostart, oend, heading)) {
					break;
				}
			}
		} else {
			calibTrigger = true;
		}

		this->m_azimuth = heading;

		#ifdef RDEBUG
		{
			// cout << "RollAng: " << RollAng << " PitchAng: " << PitchAng << endl;
			cout << "fx: " << x << " fy: " << y << " fz: " << z << endl;
			cout << "fax: " << ax << " fay: " << ay << " faz: " << az << endl;
			cout << "HeadingValue: " << HeadingValue << endl;
			cout << "m_azimuth: " << m_azimuth << endl;
		}
		#endif

		Sleep(READ_PERIOD_MS);
	}
}


void CompassDrv::calib(bool enable)
{
	m_calib = enable;
}


float CompassDrv::azimuth()
{
	return m_azimuth;
}


void CompassDrv::mapping(Axis x, Axis y, Axis z)
{
	ox = x;
	oy = y;
	oz = z;
}


void CompassDrv::filtering(int points, float coef)
{
	fx.init(points, coef); fy.init(points, coef); fz.init(points, coef);
	fax.init(points, coef); fay.init(points, coef); faz.init(points, coef);
}


CompassDrv::CompassDrv(MSensor *msensor)
{
	thr = nullptr;
	this->msensor = msensor;
	calibration = Calibration::instance();
	ox = AxisX;
	oy = AxisY;
	oz = AxisZ;
	m_calib = false;
	m_azimuth = 0;
	if (this->msensor) {
		this->msensor->init();
		lexit = false;
		thr = new LinuxThread(this, &CompassDrv::loop);
	} else {
		throw ExceptionWhat("CompassDrv msensor==nullptr");
	}
}


CompassDrv::~CompassDrv()
{
	lexit = true;
	thr->join();
	delete thr;
	delete msensor;
	delete calibration;
}

