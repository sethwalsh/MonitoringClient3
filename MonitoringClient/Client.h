#pragma once
#include "Data.h"
#include "NetClient.cpp"
#include "Account.h"
#include "error.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <LMCons.h>
#include <LM.h>
#include <WtsApi32.h>
#include <UserEnv.h>
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Wtsapi32.lib")
#include <fstream>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <boost/regex.hpp>

/*
Read Config
Set member variables
Spawn Administration Thread
Spawn Network Thread
Spawn Gather Data Thread
*/
class Client
{
public:
	Client();
	~Client();
	
	void Run();
	
	void setServer(std::string s);
	std::string getServer();
	void setPort(std::string p);
	std::string getPort();
	void readConfig(std::string file);
	void readProgramFile(std::string path);

private:

	/**
		gather - thread function handle to control all the data collection
	**/
	void gather();

	/**
		administration - thread function handle to control all the administration (account management, cleanup, logging, disk cleanup, etc)
	**/
	void administration();

	/**
		network - thread function handle to control all the network aspects of the system
	**/
	void network();	

	/**
		loggedIn - checks if the machine is currently logged in

		@return		true if logged in, false otherwise
	**/
	bool loggedIn();

	/**
		isProgramRunning - checks wether the given program name is found running in the process list

		@param	p	program name to look for
		@return		true if found, false otherwise
	**/
	bool isProgramRunning(std::string p);

	/**
		increaseProgramCount - flips the bit for the given program to the running position

		@param	p	program process name as a string
	**/
	void increaseProgramCount(std::string p);

	/**
		resetProgramCount - resets the program found count to all zeros at the end of each gathering cycle.
	**/
	void resetProgramCount();

	/**
		buildDataObject - builds a Data object for the current minute containing a user name string, time_t timestamp, and binary string of running tracked
		programs.
	**/
	void buildDataObject();

	/**
		buildPacket - converts programs found running into a binary representative string indicating running or not
	**/
	void *buildPacket();

	/**
		writeDataToDisk - writes the current minutes program data to a file on disk in case of a system crash to prevent loss of data
	**/
	void writeDataToDisk();

	/**
		getCurrentUser - return the current user logged into the machine

		@return		a string containing the currently logged in users name
	**/
	std::string getCurrentUser();

	/**
		setCurrentUser - sets the current user to the currently logged in users name

		@param		s	a string containing the user name to be set to
	**/
	void setCurrentUser(std::string s);
	
	/// Administration functions
	int accountAdministration();

	// Logoff
	void kick();

	/* Member Variables */
	bool RUNNING, LOGGED_IN_;
	NetClient *nc_;

	std::string SERVER_;
	std::string PORT_;

	// Store the currently logged in user name as a string pointer
	std::string CURRENT_USER;

	// Store a map of monitored accounts as regular expression string pointers with their associated account object that contains information about 
	// the account such as allowed / not, time length allowed, etc.
	std::vector<Account> *MONITORED_ACCOUNTS;
	std::map<std::string, time_t> *ACCOUNTS_TRACKED; // contains any users that have logged into the machine but who's time hasn't expired yet
	std::vector<std::string> *EXPIRED_ACCOUNTS; // contains any users whos time has expired

	std::vector<std::string> *PROGRAM_LIST; // contains the list of monitored programs
	std::map<std::string, int> CURRENT_PROGRAM_COUNT; // contains a map of the current program count for a given watched increment

	//std::vector<Data> *DATA_LIST; // contains a list of Data objects to either write to disk or network
	std::vector<Data*> *DATA_LIST; // contains a list of Data objects to either write to disk or network

	boost::mutex data_mtx_, li_mtx_;
};