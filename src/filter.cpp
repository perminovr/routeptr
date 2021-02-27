#include "filter.h"
#include <cfloat>


float Filter::value()
{
	float ret = val;
	if (ready) {
		ready = false;
		ret = 0.0;
		for (auto &v : values) {
			ret += v;
		}
		ret /= values.size();
		if (k > 0.001f) {
			if (val < (FLT_MIN + 1.0f)) {
				val = ret;
			} else {
				val = val * (1.0f - k) + ret * k;
			}
			ret = val;
		}
		reset();
	}
	return ret;
}


void Filter::push(float v)
{
	values.push_back(v);
	ready = (values.size() >= (size_t)points);
}


void Filter::reset()
{
	ready = false;
	values.clear();
}


bool Filter::isReady() const
{
	return ready;
}


void Filter::init(int minpoints, float k)
{
	this->points = minpoints;
	this->k = k;
}


Filter::Filter()
{
	points = 0;
	k = 0.0;
	val = FLT_MIN;
	ready = false;
}
