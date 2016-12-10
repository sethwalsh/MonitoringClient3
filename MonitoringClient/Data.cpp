#include "Data.h"

Data::Data(int pcount)
{
	MAX_PROGRAM_COUNT = pcount;

	// Set the initial EVENT to 0
	this->EVENT = new std::vector<bool>(MAX_PROGRAM_COUNT);
}

Data::~Data()
{
	delete EVENT;
}

void Data::setUser(std::string user)
{
	this->USER = user;
}

void Data::setTime()
{
	time_t temp = time(0);
	time_t t1 = temp;
	temp -= temp % 60;
	this->TIME = temp;	
}

void Data::setDataBit(int pos, bool value)
{
	this->EVENT->at(pos) = value;	
}
