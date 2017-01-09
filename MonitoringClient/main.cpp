#include "main.h"

int main(int ac, char **av)
{
	//Client *client = new Client(); // Default client config
	Client *client = new Client("config.xml"); // Use config file
	client->Run();

	return 0;
}