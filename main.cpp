
#include <iostream>
#include "Input.hpp"
#include "Server.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
		return(1);
	try
	{
		Input test(av[1]);
		test.checkFormat();
		Server s(true);

		std::cout << s << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}