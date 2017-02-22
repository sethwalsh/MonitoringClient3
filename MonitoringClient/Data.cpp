#include "Data.h"

Data::Data(int pcount)
{
	MAX_PROGRAM_COUNT = pcount;
	NETWORK_UPLOADED = false;
	// Set the initial EVENT to 0
	this->EVENT = new std::vector<bool>(MAX_PROGRAM_COUNT);
}

Data::~Data()
{
	if(EVENT != NULL)
		delete EVENT;	
}

void Data::setUser(std::string user)
{
	this->USER = user;
}

std::string Data::getUser()
{
	return this->USER;
}

void Data::setTime()
{
	time_t temp = time(0);
	time_t t1 = temp;
	temp -= temp % 60;
	this->TIME = temp;	
}

time_t Data::getTime()
{
	return this->TIME;
}

std::vector<bool>* Data::getData()
{
	return this->EVENT;
}

void Data::setDataBit(int pos, bool value)
{
	this->EVENT->at(pos) = value;	
}

void Data::setNetworkUploaded(bool b)
{
	this->NETWORK_UPLOADED = b;
}

bool Data::getNetworkUploaded()
{
	return this->NETWORK_UPLOADED;
}
