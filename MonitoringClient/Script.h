#pragma once
#include <string>

struct Script
{
	std::string _name;
	int _id;
	int _hour;
	int _minute;
	bool _repeat;
	int _weekdays[6]{ 0 };
};