#include "calibration.h"
#include <iostream>
#include <fstream>
#include "exceptionwhat.h"

using namespace std;


bool Calibration::read()
{
	if (fileName.length()) {
		ifstream f;
		f.open(fileName);
		if (f.is_open()) {
			string line;
			int size = 0;
			try {
				if (getline(f, line)) size = stoi(line);
				if (size) {
					std::vector <float> points(size);
					std::vector <float> values(size);
					for (int i = 0; i < size; ++i) {
						if (getline(f, line)) points[i] = stof(line);
						if (getline(f, line)) values[i] = stof(line);
					}
					set(points, values);
				}
			} catch (exception &ex) {
				throw ExceptionWhat("Couldn't parse file: " + fileName);
			}
			f.close();
			return (size != 0);
		}
	}
	return false;
}


bool Calibration::save()
{
	if (fileName.length()) {
		ofstream f;
		f.open(fileName);
		if (f.is_open()) {
			mu.lock();
			int size = points.size()-1; // first == last
			f << to_string(size) << endl;
			for (int i = 0; i < size; ++i) {
				f << to_string(points[i]) << endl;
				f << to_string(values[i]) << endl;
			}
			mu.unlock();
			f.close();
			return true;
		}
	}
	return false;
}


void Calibration::get(vector <float> &points, vector <float> &values)
{
	mu.lock();
	points = this->points;
	values = this->values;
	mu.unlock();
}


bool Calibration::set(const vector <float> &points, const vector <float> &values)
{
	if (points.size() == values.size()) {
		mu.lock();
		this->points = points;
		this->values = values;
		this->points.push_back(this->points[0]);
		this->values.push_back(this->values[0]);
		mu.unlock();
		return true;
	}
	return false;
}


void Calibration::setFileName(const string &fileName)
{
	this->fileName = fileName;
}


Calibration *Calibration::self = nullptr;


Calibration *Calibration::instance()
{
	if (!self)
		self = new Calibration();
	return self;
}


Calibration::~Calibration()
{
	Calibration::self = nullptr;
}