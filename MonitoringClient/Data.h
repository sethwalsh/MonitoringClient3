#include <cstdint>
#include <string>
#include <vector>
#include <time.h>

using namespace std;

class Data
{
public:
	Data(int pcount);	
	~Data();

	void setUser(std::string user);
	void setTime();
	void setDataBit(int pos);

private:
	std::vector<bool> *EVENT;
	std::string USER;
	int MAX_PROGRAM_COUNT = 0;
	time_t TIME;
};