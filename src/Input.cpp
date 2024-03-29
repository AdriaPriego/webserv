
#include "Input.hpp"
#include "Utils.hpp"
#include "Colors.hpp"

Input::Input( void ) { correct = 0; }

Input::Input( char *fileToOpen )
{
	fileOpen = fileToOpen;
	reopenFile();
	correct = 0;
}

void Input::reopenFile ( void )
{
	file.open(fileOpen, std::ios::in);

	if (!file.is_open())
		throw std::invalid_argument("File inaccessible");
}

void Input::eraseAllTabs(std::string& str, const std::string& token) {
    size_t pos = 0;
    while ((pos = str.find(token, pos)) != std::string::npos) {
        str[pos] = ' ';
    }
}

void Input::throwExeptionFormat(std::string str, int i)
{
	std::stringstream ss;
	ss << RED "File format error in line " << i << ": " NC << str;
	throw std::invalid_argument(ss.str());
}

int	Input::limitsNum(std::string num, int min, int max)
{

	if (num[num.size() - 1] == ';')
		num.erase(num.size() - 1);
	else if (min != 400)
		return (0);
	if (isInt(num))
	{
		int number = atoi(num.c_str());
		if (number > max || number < min)
			return (0);
		return (1);
	}
	return (0);
}

int Input::checkValidNames(std::vector<std::string> lineSplit)
{
	std::vector<std::string>::iterator it = lineSplit.begin();
	
	std::string str = lineSplit[lineSplit.size() - 1];

	if (str[str.size() - 1] == ';')
		lineSplit[lineSplit.size() - 1].erase(str.size() - 1);
	else
		return (0);
	if (lineSplit.size() == 2 && lineSplit[lineSplit.size() - 1].size() == 0)
		return (0);
	it++;
	for (; it != lineSplit.end(); it++)
	{
		for (int i = 0; (*it)[i]; i++)
		{
			if(isalnum((*it)[i]) == false && (*it)[i] != '.' && (*it)[i] != '-')
				return (0);
		}
	}
	return (1);
}

int Input::checkValidSize(std::string str)
{
	int type = BYTES;

	if (str[str.size() - 1] == ';')
		str.erase(str.size() - 1);
	else
		return (0);
	if (str[str.size() - 1] == 'M' || str[str.size() - 1] == 'K' || str[str.size() - 1] == 'G')
	{
		if (str[str.size() - 1] == 'K')
			type = KILOS;
		else if (str[str.size() - 1] == 'M')
			type = MEGAS;
		else if (str[str.size() - 1] == 'G')
			type = GIGAS;
		str.erase(str.size() - 1);
	}
	if (isInt(str))
	{
		long num = atoi(str.c_str());
		for (int i = 0; i < type; i++)
			num *= 1024;
		//AQUI ESTA EN BYTES TO SAVE IN A CLASS 
		return(1);
	}
	return(0);
}

int Input::checkValidDir(std::string str)
{
	if (str[str.size() - 1] == ';')
		str.erase(str.size() - 1);
	else
		return (0);
	if (str.size() == 0)
		return (0);
	for (int i = 0; str[i]; i++)
	{
		if(isalnum(str[i]) == false && str[i] != '.' && str[i] != '-' && str[i] != '_' && str[i] != '/')
			return (0);
	}
	return (1);
}

int Input::checkServSplit(std::string str, int flag, bool key_open, int i)
{
	std::vector<std::string> lineSplit = split(str, ' ');

	if (lineSplit.size() > 0 && lineSplit[0].find('#') == 0)
		return (flag);
	if (key_open == false)
	{
		if (lineSplit.size() == 1 && lineSplit[0] == "server" && (flag == NOT_INIT || flag == EMPTY))
			return(MISS_KEY);
		else if (lineSplit.size() == 1 && lineSplit[0] == "server{" && (flag == NOT_INIT || flag == EMPTY))
			return (OPEN_KEY);
		else if (lineSplit.size() == 1 && lineSplit[0] == "{" && flag == MISS_KEY)
			return (OPEN_KEY);
		else if (lineSplit.size() == 2 && lineSplit[0] == "server" && lineSplit[1] == "{")
			return(OPEN_KEY);
	}
	else if (key_open == true)
	{
		if (lineSplit.size() == 2 && lineSplit[0] == "listen" && limitsNum(lineSplit[1], 0, 65535))
			return (VALID_ARG);
		if (lineSplit.size() >= 2 && lineSplit[0] == "server_name" && checkValidNames(lineSplit))
			return	(VALID_ARG);
		if (lineSplit.size() == 2 && lineSplit[0] == "client_max_body_size" && checkValidSize(lineSplit[1]))
			return	(VALID_ARG);
		if (lineSplit.size() == 2 && lineSplit[0] == "root" && checkValidDir(lineSplit[1]))
			return (VALID_ARG);
		if (lineSplit.size() == 3 && lineSplit[0] == "error_page" && limitsNum(lineSplit[1], 400, 599) && checkValidDir(lineSplit[2]))
			return (VALID_ARG);
		if (lineSplit.size() == 2 && lineSplit[0] == "upload_store" && checkValidDir(lineSplit[1]))
			return (VALID_ARG);
		if (lineSplit.size() == 1 && lineSplit[0] == "}")
			return (CLOSE_KEY);
	}
	if (lineSplit.size() == 0)
		return (EMPTY);
	throwExeptionFormat(str, i);
	return (KO);
}

bool Input::checkFormat( void )
{
	std::string line;
	int flag = NOT_INIT;
	bool key_open = false;
	int i = 1;
	do
	{
		std::getline(file, line);
		eraseAllTabs(line, "\t");
		flag = checkServSplit(line, flag, key_open, i);
		if (flag == OPEN_KEY)
		{
			key_open = true;
		}
		else if (flag == CLOSE_KEY)
		{
			key_open = false;
			std::cout << GREEN "SERVER CREATED" NC << std::endl;
		}
		i++;
	} while (!file.eof());

	return (1);
}

Input::~Input( void )
{
	if (file.is_open())
		file.close();
}