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
	std::string getUser();
	void setTime();
	time_t getTime();
	std::vector<bool>* getData();
	void setDataBit(int pos, bool value);
	void setNetworkUploaded(bool b);
	bool getNetworkUploaded();

private:
	std::vector<bool> *EVENT;
	std::string USER;
	int MAX_PROGRAM_COUNT = 0;
	time_t TIME;
	bool NETWORK_UPLOADED;
};