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

	/// Default config settings are being used so statically set them here
	this->setServer("localhost");
	this->setPort("16000");
	this->readConfig("G:\\MonitoringClient\\Debug\\config"); // any settings found in config can now override their default state		
}

Client::Client(std::string cpath)
{
	this->RUNNING = true;
	this->MONITORED_ACCOUNTS = new std::vector<Account>();
	this->EXPIRED_ACCOUNTS = new std::vector<std::string>();
	this->ACCOUNTS_TRACKED = new std::map<std::string, time_t>();
	this->PROGRAM_LIST = new std::vector<std::string>();
	//this->DATA_LIST = new std::vector<Data>();
	this->DATA_LIST = new std::vector<Data*>();
	this->SCRIPT_LIST = new std::vector<Script>();

	// Read configuration settings and set the appropriate members
	this->readConfig(cpath);	
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
		//std::cout << "New GATHER cycle." << std::endl;
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
	//Account a;
	//a.BLOCKED = FALSE;
	//a.BLOCKED = TRUE;
	//a.NAME_REGEX = "Se";
	//a.HOUR = 1;

	//this->MONITORED_ACCOUNTS->push_back(a);

	//std::vector<Account> *MONITORED_ACCOUNTS;
	//std::map<std::string, time_t> *ACCOUNTS_TRACKED; // contains any users that have logged into the machine but who's time hasn't expired yet
	//std::vector<std::string> *EXPIRED_ACCOUNTS; // contains any users whos time has expired
	int _ERROR_CODE = 0x0;

	while (RUNNING)
	{	
		//std::cout << "NEW ADMIN LOOP CYCLE" << std::endl;

		if (!LOGGED_IN_)
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		else
		{
			_ERROR_CODE = this->accountAdministration();

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
					//std::cout << "Your account is blocked on this machine and I cannot allow you to log in." << std::endl;
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
			//std::cout << "Your account time limit has already expired on this machine, or another, and you are not allowed to log back in." << std::endl;
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
				//std::cout << "Your account time limit has expired and you will be kicked from the machine now." << std::endl;
				this->EXPIRED_ACCOUNTS->push_back(_u);

				// Kick the account
				kick();

				return _RETURN;
			}
			if (_cTime - (15 * 60) >= _track_itr->second)
			{
				//std::cout << "Your account time limit has 15 minutes left before I will log you out from this machine." << std::endl;

				// TODO::: Set some sort of message flag 

				return _RETURN;
			}
			if (_cTime - (5 * 60) >= _track_itr->second)
			{
				//std::cout << "Your account time limit has 5 minutes left before I will log you out from this machine." << std::endl;

				// TODO::: Set some soft of message flag

				return _RETURN;
			}
		}
		else
		{
			// Add to ACCOUNTS_TRACKED
			/**
			_current is NULL ...error!
			**/
			if(_current == NULL)
				_current = new Account();

			//std::cout << "This is the first time I've encountered your Account and it is a monitored account so I will begin tracking your logged in time from now until expiration." << std::endl;
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

		/// Build vector of data objects from disk 
		///	lock file 
		/// send each object to server
		/// clear file after confirmation
		if (!this->DATA_LIST->empty())
		{
			boost::asio::io_service io_service;
			tcp::resolver r(io_service);
			NetClient nc_(io_service);

			//this->data_mtx_.lock();
			std::vector<unsigned char> *packets_ = buildPacket(1);
			std::string hash = this->md5HashString(packets_, packets_->size());
			for (int i = 0; i < hash.length(); i++)
			{
				unsigned char c_ = hash.at(i);
				packets_->push_back(c_);
			}
			nc_.start(r.resolve(tcp::resolver::query(this->getServer(), this->getPort())));
			io_service.run();
		}		

		boost::this_thread::sleep(boost::posix_time::milliseconds(500));		
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
					//std::cout << "Program: " << pexe_ << " Process: " << s << std::endl;
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

std::vector<unsigned char> * Client::buildPacket(uint16_t flag)
{
	/**
		Total size -- 4096 bytes per packet

		Flag	1
		Size	3
		Payload	N
		MD5		32
	**/
	if (this->DATA_LIST->empty())
		return NULL;
	std::stringstream sstr;
	std::vector<unsigned char> *PACKETS_ = new std::vector<unsigned char>();
	//std::vector<unsigned char> PACKET_;

	PACKETS_->insert(PACKETS_->begin() + 0, flag & 0xFF);
	PACKETS_->insert(PACKETS_->begin() + 1, flag >> 8);
		
	std::vector<Data*>::iterator it;
	this->data_mtx_.lock();
	for (it = this->DATA_LIST->begin(); it != this->DATA_LIST->end(); it++)
	{
		if ((*it)->getNetworkUploaded())
			continue;		

		std::vector<unsigned char> EVENT;		
		std::string timestr_, eventstr_, userstr_;

		uint16_t eflag_ = 3;		
		
		/// TIMESTAMP
		sstr << (*it)->getTime();
		timestr_ = sstr.str();
		uint16_t tsize_ = timestr_.length();		
		sstr.str(std::string());
		
		/// EVENT data (program data, etc)
		std::string out("");
		std::vector<bool> v = *(*it)->getData();
		out.reserve(v.size());
		for (bool b : v)
		{
			out += b ? '1' : '0';
		}
		//sstr << (*it)->getData();
		eventstr_ = out;//sstr.str();
		uint16_t esize_ = eventstr_.length();
		//sstr.str(std::string());
		
		/// USER NAME
		sstr << (*it)->getUser();
		userstr_ = sstr.str();
		uint16_t usize_ = userstr_.length();
		sstr.str(std::string());

		uint16_t psize_ = tsize_ + esize_ + usize_;

		/// uint16_t value = 12345;
		/// char low = value & 0xFF;
		/// char hi = value >> 8;
		/// To reverse: uint16_t value = low | uint16_t(hi) << 8;
		EVENT.insert(EVENT.begin() + 0, eflag_ & 0xFF);//EVENT.at(0) = eflag_ & 0xFF;
		EVENT.insert(EVENT.begin() + 1, eflag_ >> 8);//EVENT.at(1) = eflag_ >> 8;
		EVENT.insert(EVENT.begin() + 2, psize_ & 0xFF);
		EVENT.insert(EVENT.begin() + 3, psize_ >> 8);
		EVENT.insert(EVENT.begin() + 4, usize_ & 0xFF);
		EVENT.insert(EVENT.begin() + 5, usize_ >> 8);
		EVENT.insert(EVENT.begin() + 6, tsize_ & 0xFF);
		EVENT.insert(EVENT.begin() + 7, tsize_ >> 8);
		
		int i = 8;
		for (int j = 0; j < esize_; j++)
		{
			EVENT.insert(EVENT.begin() + i, (unsigned char)eventstr_.at(j));
			i++;
		}

		for (int j = 0; j < tsize_; j++)
		{
			EVENT.insert(EVENT.begin() + i, (unsigned char)timestr_.at(j));
			i++;
		}

		for (int j = 0; j < usize_; j++)
		{
			EVENT.insert(EVENT.begin() + i, (unsigned char)userstr_.at(j));
			i++;
		}
		
		PACKETS_->insert(PACKETS_->end(), EVENT.begin(), EVENT.end());
		(*it)->setNetworkUploaded(true);
	}
	this->data_mtx_.unlock();	

	return PACKETS_;
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

	// If the element at back of list has been added to the network then it is safe to remove it since it was also just written to disk backup
	if (this->DATA_LIST->back()->getNetworkUploaded())
		this->DATA_LIST->pop_back();

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
	try {
		boost::property_tree::ptree _pt;
		boost::property_tree::xml_parser::read_xml(file, _pt);

		/**
		Server
		Port
		Log file
			- log file size limit
			- backup or delete
		Users
			- Restrictions
			- Blocked
			- Messages
		Scripts
			- Script path
			- Script time
		**/
		this->SERVER_ = _pt.get<std::string>("server");
		this->PORT_ = _pt.get<std::string>("port");//_pt.get<int>("port");
		this->LOG_FILE_ = _pt.get<std::string>("log");
		
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, _pt.get_child("Users")) {			
			Account *_a = new Account();
			if (v.first == "Account")
			{
				char *_nregex = new char[v.second.get_child("name").data().length() +1];
				strcpy(_nregex, v.second.get_child("name").data().c_str());//_a->NAME_REGEX = v.second.get_child("name").data().c_str();
				_a->NAME_REGEX = _nregex;
				_a->BLOCKED = atoi(v.second.get_child("blocked").data().c_str());
				_a->HOUR = v.second.get<int>("hour", 2);
			}
			this->MONITORED_ACCOUNTS->push_back(*_a);
		}
		
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, _pt.get_child("Commands")) {
			Script *_s = new Script();
			int _ID = 0;
			if (v.first == "command")
			{
				_s->_id = _ID;
				
				_s->_name = v.second.get_child("path").data();

				/// Days of the week to execute the script
				string _d = v.second.get_child("day").data();
				std::vector<std::string> _days;
				boost::split(_days, _d, boost::is_any_of(":"));
				if (_days.size() == 1)
				{
					_s->_weekdays[7] = { 1 };
				}
				else
				{
					_s->_weekdays[7] = { 0 };
					std::vector<std::string>::iterator it;
					for (it = _days.begin(); it != _days.end(); it++)
					{
						int _index = stoi(*it);
						if(_index > 0)
							_s->_weekdays[_index-1] = 1;
					}
				}
				/// Time to execute the script 
				string _t = v.second.get_child("time").data();
				std::vector<std::string> _time;
				boost::split(_time, _t, boost::is_any_of(":"));
				if (_time.size() > 1)
				{
					_s->_hour = stoi(_time.at(0));
					_s->_minute = stoi(_time.at(1));
				}
				if (_time.size() == 1 && stoi(_time.at(0)) < 25)
				{
					_s->_hour = stoi(_time.at(0));
					_s->_minute = 0;
				}

				std::string _repeat = v.second.get_child("repeat").data();
				_s->_repeat = boost::iequals("False", _repeat);
				_ID++;
			}
			this->SCRIPT_LIST->push_back(*_s);
		}
	}
	catch (std::exception &e)
	{
		/// TODO:: logging
		this->log(e.what(), 0x0, 0);
		//std::cout << e.what() << std::endl;
	}
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

std::string Client::md5HashString(const void * data, const size_t size)
{
	HCRYPTPROV hProv = NULL;
	HCRYPTPROV hHash = NULL;
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		std::cout << "Failed to get context" << std::endl;
	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
		CryptReleaseContext(hProv, 0);
	}
	
	if (!CryptHashData(hHash, static_cast<const BYTE*>(data), size, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
	}
	DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&cbHashSize, &dwCount, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
	}
	std::vector<BYTE> buffer(cbHashSize);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(&buffer[0]), &cbHashSize, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
	}

	std::ostringstream oss;
	for (std::vector<BYTE>::const_iterator iter = buffer.begin(); iter != buffer.end(); ++iter) {
		oss.fill('0');
		oss.width(2);
		oss << std::hex << static_cast<const int>(*iter);
	}

	return oss.str();
}

int Client::scriptAdministration()
{
	/**
	- for every command check the time and if it is time then execute the command
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