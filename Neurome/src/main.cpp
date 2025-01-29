#include <iostream>

#include "Neurome.h"

int main(int argc, const char *argv[])
{
	if (argc != 1)
	{
		std::cerr << "ERR: Arguments are not supported" << std::endl;
		std::cout << "Example: " << argv[0] << std::endl;

		return -1;
	}

	Neurome client;
	client.start();

	return client.exit();
}
