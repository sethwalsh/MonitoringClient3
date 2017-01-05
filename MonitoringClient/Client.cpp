#include "Client.h"

Client::Client()
{
	this->RUNNING = true;
	this->MONITORED_ACCOUNTS = new std::vector<Account>();
	this->EXPIRED_ACCOUNTS = new std::vector<std::string>();
	this->ACCOUNTS_TRACKED = new std::map<std::string, time_t>();
	this->PROGRAM_LIST = new std::vector<std::string>();
	//this->DATA_LIST = new std::vector<Data>();
	this->DATA_LIST = new std::vector<Data*>();
	this->SCRIPT_LIST = new std::vector<Script>();
}

Client::Client(std::string cpath)
{
	// Read configuration settings and set the appropriate members
	this->readConfig(cpath);

	this->RUNNING = true;
	this->MONITORED_ACCOUNTS = new std::vector<Account>();
	this->EXPIRED_ACCOUNTS = new std::vector<std::string>();
	this->ACCOUNTS_TRACKED = new std::map<std::string, time_t>();
	this->PROGRAM_LIST = new std::vector<std::string>();
	//this->DATA_LIST = new std::vector<Data>();
	this->DATA_LIST = new std::vector<Data*>();
	this->SCRIPT_LIST = new std::vector<Script>();
}

Client::~Client()
{
	delete MONITORED_ACCOUNTS;
	delete EXPIRED_ACCOUNTS;
	delete ACCOUNTS_TRACKED;
	delete PROGRAM_LIST;
	delete DATA_LIST;
	delete SCRIPT_LIST;
}

void Client::Run()
{
	// Spawn 1 thread to Gather data
	boost::thread gthread_( boost::bind( &Client::gather, this ) );

	// Spawn 1 thread to Handle Network
	boost::thread nthread_( boost::bind( &Client::network, this ) );

	// Spawn 1 thread to Deal with Administration (scripts, users, logouts)
	boost::thread athread_( boost::bind( &Client::administration, this ) );

	gthread_.join();
	nthread_.join();
	athread_.join();		
}

/***
	Count running tracked programs
	Build a Data object each minute
***/
void Client::gather()
{
	//this->readProgramFile("D:\\GitHub Projects\\MonitoringClient3\\Debug\\masterlist.txt");
	this->readProgramFile("G:\\MonitoringClient\\Debug\\masterlist.txt");

	time_t _current = time(0);
	struct tm *now = localtime(&_current);
	int _CURRENT_MINUTE = now->tm_min;

	while (RUNNING)
	{
		std::cout << "New GATHER cycle." << std::endl;
		//log("test", 1, 1); log ERROR with error code 1
		//log("test", 0, 0); log WARN with no error code
		//log("test", 0, 1); log WARN with error code 1

		if ( this->loggedIn() )
		{
			_current = time(0);
			now = localtime(&_current);
			if (now->tm_min != _CURRENT_MINUTE)
			{
				// Build a Data object for the current minute
				this->buildDataObject();

				// Write Data to disk in case of program failure which would cause a loss of data that had not been uploaded yet
				this->writeDataToDisk();

				// Reset the program counters for the next cycle
				this->resetProgramCount();				

				// Reset the current minute
				_CURRENT_MINUTE = now->tm_min;
			}
			/*
			- Get current programs in use
				- Create data event
				- Add data to list to be sent off to the server at the next network send
			*/
			for (int i = 0; i < this->PROGRAM_LIST->size(); ++i)
			{
				if (this->isProgramRunning(this->PROGRAM_LIST->at(i)))
				{
					// Flip the programs bit
					this->increaseProgramCount(this->PROGRAM_LIST->at(i));
				}
			}			
		}
		else
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}	
}

void Client::administration()
{	
	Account a;
	a.BLOCKED = FALSE;
	//a.BLOCKED = TRUE;
	a.NAME_REGEX = "Se";
	a.HOUR = 1;

	this->MONITORED_ACCOUNTS->push_back(a);

	//std::vector<Account> *MONITORED_ACCOUNTS;
	//std::map<std::string, time_t> *ACCOUNTS_TRACKED; // contains any users that have logged into the machine but who's time hasn't expired yet
	//std::vector<std::string> *EXPIRED_ACCOUNTS; // contains any users whos time has expired
	int _ERROR_CODE = 0x0;

	while (RUNNING)
	{	
		std::cout << "NEW ADMIN LOOP CYCLE" << std::endl;

		if (!LOGGED_IN_)
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		else
		{
			//_ERROR_CODE = this->accountAdministration();

			//_ERROR_CODE = this->scriptAdministration();

			//_ERROR_CODE = this->diskCleanup();			
			/**
			- scripts
			- disk space
			- guest accounts
			- messaging
			**/
		}
		Sleep(5000);
	}

	// Shutdown any administration specific stuff here	
}

int Client::accountAdministration()
{
	int _RETURN = 0x0;

	try {
		Account *_current = NULL;
		std::string _u = getCurrentUser();
		if (_u.length() < 1)
			return _RETURN;
				
		/// BLOCKED
		for (int i = 0; i < this->MONITORED_ACCOUNTS->size(); i++)
		{
			boost::regex _account(MONITORED_ACCOUNTS->at(i).NAME_REGEX, boost::regex::perl | boost::regex::icase);
			boost::match_results<std::string::const_iterator> _results;
			std::string::const_iterator start = _u.begin();
			std::string::const_iterator end = _u.end();
			if (boost::regex_search(start, end, _results, _account))
			{
				_current = &(this->MONITORED_ACCOUNTS->at(i));

				if (_current->BLOCKED)
				{
					std::cout << "Your account is blocked on this machine and I cannot allow you to log in." << std::endl;
					kick();

					return _RETURN;
				}				
			}
		}
		//if (_current != NULL && _current->BLOCKED)
		//{
		//	std::cout << "Your account is blocked on this machine and I cannot allow you to log in." << std::endl;
		//	kick();
		//}

		/// EXPIRED
		std::vector<std::string>::iterator _exp_it = std::find(this->EXPIRED_ACCOUNTS->begin(), this->EXPIRED_ACCOUNTS->end(), _u);
		if (_exp_it != this->EXPIRED_ACCOUNTS->end())
		{
			std::cout << "Your account time limit has already expired on this machine, or another, and you are not allowed to log back in." << std::endl;
			// Kick the account
			kick();

			return _RETURN;
		}

		/// TRACKED
		std::map<std::string, time_t>::iterator _track_itr = this->ACCOUNTS_TRACKED->find(_u);
		if (_track_itr != this->ACCOUNTS_TRACKED->end())
		{
			time_t _cTime = time(0);
			/// Display messages in whatever time sequence you decide here, so any future changes will need to be accounted for here
			if (_cTime >= _track_itr->second)
			{
				std::cout << "Your account time limit has expired and you will be kicked from the machine now." << std::endl;
				this->EXPIRED_ACCOUNTS->push_back(_u);

				// Kick the account
				kick();

				return _RETURN;
			}
			if (_cTime - (15 * 60) >= _track_itr->second)
			{
				std::cout << "Your account time limit has 15 minutes left before I will log you out from this machine." << std::endl;

				// TODO::: Set some sort of message flag 

				return _RETURN;
			}
			if (_cTime - (5 * 60) >= _track_itr->second)
			{
				std::cout << "Your account time limit has 5 minutes left before I will log you out from this machine." << std::endl;

				// TODO::: Set some soft of message flag

				return _RETURN;
			}
		}
		else
		{
			// Add to ACCOUNTS_TRACKED
			std::cout << "This is the first time I've encountered your Account and it is a monitored account so I will begin tracking your logged in time from now until expiration." << std::endl;
			this->ACCOUNTS_TRACKED->insert(
				std::pair<std::string, time_t>(
					_u, 
					time(0) + (_current->HOUR*3600)
					)
			);
		}
		_RETURN = static_cast<int>(GetLastError());
	}
	catch (std::exception &e) {
		_RETURN = static_cast<int>(GetLastError());
		return _RETURN;
	}

	return _RETURN;
}

void Client::network()
{
	/// TODO - implement Networking after we have a reliable event and gather function
	
	while (RUNNING)
	{		
		if (!LOGGED_IN_)
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		else
		{
		}
		/*
		boost::asio::io_service io_service;
		tcp::resolver r(io_service);
		NetClient nc_(io_service);
		nc_.start(r.resolve(tcp::resolver::query(this->getServer(), this->getPort())));
		io_service.run();

		boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		*/
	}	
}

bool Client::loggedIn()
{
#ifdef _WIN32
	std::string username = "";
	char szTempBuf[MAX_PATH] = { 0 };

	HANDLE hToken = NULL;
	HANDLE hDupToken = NULL;
	// Get the user of the "active" TS session
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	if (0xFFFFFFFF == dwSessionId)
	{
		// there is no active session
		this->li_mtx_.lock();
		this->LOGGED_IN_ = FALSE;
		this->li_mtx_.unlock();
	}
	WTSQueryUserToken(dwSessionId, &hToken);
	if (NULL == hToken)
	{
		// function call failed
	}
	DuplicateToken(hToken, SecurityImpersonation, &hDupToken);
	if (NULL == hDupToken)
	{
		CloseHandle(hToken);
	}
	BOOL bRes = ImpersonateLoggedOnUser(hDupToken);
	if (bRes)
	{
		// Get the username
		DWORD dwBufSize = sizeof(szTempBuf);
		bRes = GetUserNameA(szTempBuf, &dwBufSize);
		RevertToSelf(); // stop impersonating the user
		if (bRes)
		{
			// the username string is in szTempBuf
		}
	}
	CloseHandle(hDupToken);
	CloseHandle(hToken);

	/** REMOVING THIS WHILE DEBUGGIN **/
	//username = szTempBuf;

	/** Remove below code when finished debugging because you can't get the correct user name otherwise **/
	char un[UNLEN+1];
	DWORD username_size = sizeof(un);
	GetUserName(un, &username_size);
	username = un;

	this->setCurrentUser(username);

	/** When done debugging remove the above stuff ^^ **/
	if (username.length() > 0)
	{
		this->li_mtx_.lock();
		this->LOGGED_IN_ = TRUE;
		this->li_mtx_.unlock();
	}
#endif
#ifdef __linux__
#endif
	return LOGGED_IN_;
}

bool Client::isProgramRunning(std::string p)
{
#ifdef _WIN32
	char *compare;
	std::string pexe_;
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pexe_ = p +".exe";

	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	pe32.dwSize = sizeof(PROCESSENTRY32);
	// Get first running process
	if (Process32First(hProcessSnap, &pe32))
	{
		if (pe32.szExeFile == pexe_)
		{
			return true;
		}
		else
		{
			// Loop through all running processes looking for the given process
			while (Process32Next(hProcessSnap, &pe32))
			{
				compare = pe32.szExeFile;
				std::string s = std::string(compare);

				boost::regex _program("^" + pexe_, boost::regex::perl | boost::regex::icase);
				boost::match_results<std::string::const_iterator> _results;
				std::string::const_iterator start = s.begin();
				std::string::const_iterator end = s.end();
				if (boost::regex_search(start, end, _results, _program))
				{
					std::cout << "Program: " << pexe_ << " Process: " << s << std::endl;
					return true;
				}

				//if (pexe_.compare(s) == 0)
				//{
					// if found running, set to true and break loop
				//	return true;
				//	break;
				//}
			}
		}
		CloseHandle(hProcessSnap);
	}
#endif
	return false;
}

void Client::increaseProgramCount(std::string p)
{
	this->CURRENT_PROGRAM_COUNT[p] = 1;
}

void Client::resetProgramCount()
{
	std::map<std::string, int>::iterator it;
	for (it = this->CURRENT_PROGRAM_COUNT.begin(); it != this->CURRENT_PROGRAM_COUNT.end(); it++)
		it->second = 0;
}

void Client::buildDataObject()
{
	Data *d = new Data(this->PROGRAM_LIST->size());
	d->setTime();	
	d->setUser(this->CURRENT_USER);

	std::map<std::string, int>::iterator it;
	for (it = this->CURRENT_PROGRAM_COUNT.begin(); it != this->CURRENT_PROGRAM_COUNT.end(); it++)
	{
		int _pos = std::find(this->PROGRAM_LIST->begin(), this->PROGRAM_LIST->end(), it->first) - this->PROGRAM_LIST->begin();		
		if (_pos < this->PROGRAM_LIST->size())
			d->setDataBit(_pos, it->second);	
	}
	
	// Push the Data object onto the list
	this->data_mtx_.lock();
	this->DATA_LIST->push_back(d);
	this->data_mtx_.unlock();
}

void * Client::buildPacket()
{
	char *PACKET_;
	return (void*)PACKET_;
}

void Client::writeDataToDisk()
{
	this->data_mtx_.lock();
	time_t temp = this->DATA_LIST->back()->getTime();
	this->data_mtx_.unlock();
	
	std::ofstream save("saved_data.txt", std::ios_base::app | std::ios_base::out);

	unsigned int _DATA = 0;
	save << temp << "," << this->getCurrentUser() << ",";
	int count = 0;

	this->data_mtx_.lock();
	std::vector<bool>::iterator it;	
	std::vector<bool> *_tempData = this->DATA_LIST->back()->getData();
	for (it = _tempData->begin(); it != _tempData->end(); it++)
	{
		if ((*it) == TRUE)
		{
			save << "1";
		}
		else
			save << "0";
		count++;
	}	
	this->data_mtx_.unlock();
	save << "\n";	
}

std::string Client::getCurrentUser()
{
	return this->CURRENT_USER;
}

void Client::setCurrentUser(std::string s)
{
	this->CURRENT_USER = s;
}

void Client::kick()
{
	SetLastError(0);
	if (!WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE, WTSGetActiveConsoleSessionId(), FALSE))
	{
		//log("Failed to logoff user.", 1, 0);
	}
}

void Client::setServer(std::string s)
{
	this->SERVER_ = s;
}

std::string Client::getServer()
{
	return this->SERVER_;
}

void Client::setPort(std::string p)
{
	this->PORT_ = p;
}

std::string Client::getPort()
{
	return this->PORT_;
}

void Client::readConfig(std::string file)
{
}

void Client::readProgramFile(std::string path)
{
	try {
		std::ifstream file(path);
		std::string program;
		while (std::getline(file, program))
		{
			if (!program.empty())
			{
				this->PROGRAM_LIST->push_back(program);
				this->CURRENT_PROGRAM_COUNT[program] = 0;
			}
			if (program.empty())
			{
				this->PROGRAM_LIST->push_back("NOT_A_PROGRAM_FOR_THIS_OS");
				this->CURRENT_PROGRAM_COUNT["NOT_A_PROGRAM_FOR_THIS_OS"] = 0;
			}
		}
	}
	catch (std::exception &e)
	{

	}
}

int Client::scriptAdministration()
{
	/**
	- for every script check the time and if it is time then execute the script
	**/
	for (int i = 0; i < this->SCRIPT_LIST->size(); i++)
	{
		time_t t = time(0);
		struct tm *now = localtime(&t);
		Script s = this->SCRIPT_LIST->at(i);

		if (now->tm_hour == s._hour && now->tm_min == s._minute)
		{
			// Sunday == 0 ... Saturday == 6
			if (s._weekdays[now->tm_wday] == 1)
			{
				// current minute, hour, day matched so execute this script

			}
		}
	}
	return 0;
}

void Client::log(std::string emsg, int ecode, int level)
{
	try {
		std::ofstream log(LOG_FILE_, std::ios_base::app | std::ios_base::out);
		time_t t = time(0);
		struct tm *now = localtime(&t);

		// Level = 1 is an Error, all other levels are a Warning
		if (level == 1)
			log << "ERROR    " << emsg << "    ";
		else
			log << "WARNING    " << emsg << "    ";
		if (ecode > 0)
			log << ecode << "    ";
		log << asctime(now);		
	}
	catch (std::exception &e)
	{

	}
}