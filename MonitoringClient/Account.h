#pragma once
#include <string>
#include <ctime>

struct Account
{
	const char *NAME_REGEX;
	bool BLOCKED;
	int HOUR;	
};