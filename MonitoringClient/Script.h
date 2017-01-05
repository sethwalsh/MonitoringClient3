#pragma once
#include <string>

struct Script
{
	std::string _name;
	int _id;
	int _hour;
	int _minute;
	int _weekdays[6]{ 0 };
};