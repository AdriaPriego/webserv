
#include <poll.h>
#include "Headers.hpp"
#include "Signals.hpp"
#include "StartServer.hpp"
#include <dirent.h>

/* IMPORTANT THINKS:
		- ERROR PAGE FROM ROOT (NOT DONE)
*/

int	createNewSocket(t_listen & list)
{
	std::string		ip = list.ip;
	std::string		port = toString(list.port);
	int				localSocket, errGai, option;
	struct addrinfo	*addr;
	struct addrinfo	hints;
	
	if (ip.empty())
		ip = "localhost";
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((errGai = getaddrinfo(ip.c_str(), port.c_str(), &hints, &addr)) != 0)
	{
		throw std::runtime_error(gai_strerror(errGai));
	}
	if ((localSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0)
	{
		freeaddrinfo(addr);
		throw std::runtime_error(strerror(errno));
	}
	if (setsockopt(localSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
	{
		freeaddrinfo(addr);
		throw std::runtime_error(strerror(errno));
	}
	if (fcntl(localSocket, F_SETFL, O_NONBLOCK | FD_CLOEXEC) < 0)
	{
		freeaddrinfo(addr);
		throw std::runtime_error(strerror(errno));
	}
	if (bind(localSocket, addr->ai_addr, addr->ai_addrlen) != 0)
	{
		sockaddr_in* sockAddrIn = reinterpret_cast<sockaddr_in*>(addr->ai_addr);
		int port = ntohs(sockAddrIn->sin_port);
		std::string	mssgErr = strerror(errno);
		mssgErr += ": Port -> ";
		mssgErr += toString(port);
		freeaddrinfo(addr);
		throw std::runtime_error(mssgErr);
	}
	if (listen(localSocket, MAX_CONNECTION_BACKLOG) != 0)
	{
		freeaddrinfo(addr);
		throw std::runtime_error(strerror(errno));
	}
	freeaddrinfo(addr);
	return (localSocket);
}

std::vector<socketServ>	initSockets(std::vector<Server> & s)
{
	int						sId = 0;
	socketServ				socks;
	std::vector<t_listen>	cListen;
	std::vector<socketServ>	sockets;

	for (std::vector<Server>::iterator itServ = s.begin(); itServ != s.end(); itServ++)
	{
		Server &	currentServ = (*itServ);

		socks.serv = currentServ;
		cListen = currentServ.getListen();
		std::cout << getTime() << BOLD GREEN "Server #" << sId << " running:" NC << std::endl;
		for (std::vector<t_listen>::iterator itListen = cListen.begin(); itListen != cListen.end(); itListen++)
		{
			try
			{
				int	fd = createNewSocket(*itListen);
				socks.servSock = fd;
				sockets.push_back(socks);
				std::cout << PURPLE "\t\t\tListening: " << (*itListen).ip << ":" << (*itListen).port << NC << std::endl;
			}
			catch(const std::exception& e)
			{
				std::cerr << getTime() << BOLD RED << e.what() << NC << std::endl;
			}
		}
		sId++;
	}
	return (sockets);
}

int createDirectory(Response &res, std::string dir)
{
    DIR *opened = opendir(dir.c_str());
	struct dirent *entry;
	std::string body;

	if (!opened)
		return (1);

	std::cout << "HIIHIHIHIHI" << std::endl;
	entry = readdir(opened);

	body = "<h1>Directory: " + dir + "</h1>";
	while(entry)
	{
		body += "<li>";
		body += entry->d_name;
		body += "</li>\n";
		entry = readdir(opened);
	}
	closedir(opened);
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_TYPE, "text/html"));
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_LENGTH, toString(body.size())));
	res.setBody(body);
	return(0);
}

int createResponseImage( std::string fileToOpen, Response &res)
{
	std::ifstream fileicon(fileToOpen, std::ios::binary);

	if (!fileicon.is_open())
		return (1);

	std::string html;
	char c;

	while (fileicon.get(c))
		html += c;
	fileicon.close();
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_TYPE, "image/x-icon"));
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_LENGTH, toString(html.size())));
	res.setBody(html);
	return (0);
}

int createResponseHtml( std::string fileToOpen, Response &res)
{
	std::ifstream file(fileToOpen);

	if (!file.is_open())
		return (1);
	std::string html;
	std::cout << fileToOpen << std::endl;

	getline(file, html, '\0');
	file.close();
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_TYPE, "text/html"));
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_LENGTH, toString(html.size())));
	res.setBody(html);
	return (0);
}

int createResponseError( Response &res, int codeError, std::map<int, std::string> errorPageServ)
{
	res.setStatusLine((statusLine){"HTTP/1.1", codeError, ERROR_MESSAGE(codeError)});
	if (!errorPageServ.empty() && errorPageServ.find(codeError) != errorPageServ.end())
	{
		if (createResponseHtml(errorPageServ[codeError], res) == 0)
			return (0);
	}
	std::string body;

	body += "<h1 style=\"text-align: center;\">" + toString(codeError) + " " + ERROR_MESSAGE(codeError) + "</h1>";
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_TYPE, "text/html"));
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_LENGTH, toString(body.size())));
	res.setBody(body);
	return (0);
}

int createResponseError( Response &res, int codeError, std::map<int, std::string> errorPageServ, std::map<int, std::string> errorPageLoc)
{
	res.setStatusLine((statusLine){"HTTP/1.1", codeError, ERROR_MESSAGE(codeError)});
	if (!errorPageLoc.empty() && errorPageLoc.find(codeError) != errorPageLoc.end())
	{
		if (createResponseHtml(errorPageLoc[codeError], res) == 0)
			return (0);
	}
	else if (!errorPageServ.empty() && errorPageServ.find(codeError) != errorPageServ.end())
	{
		if (createResponseHtml(errorPageServ[codeError], res) == 0)
			return (0);
	}
	std::string body;

	body += "<h1 style=\"text-align: center;\">" + toString(codeError) + " " + ERROR_MESSAGE(codeError) + "</h1>";
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_TYPE, "text/html"));
	res.addHeaderField(std::pair<std::string, std::string>(CONTENT_LENGTH, toString(body.size())));
	res.setBody(body);
	return (0);
}

int test(Request req)
{
	std::map<std::string, std::string> headers = req.getHeaderField();

	std::string acceptTypes = headers["accept"];

	if (acceptTypes.find("text/html") != acceptTypes.npos)
		return (1);
	else if (acceptTypes.find("image/") != acceptTypes.npos)
		return (2);
	else if (acceptTypes.find("*/*") != acceptTypes.npos)
		return (1);
	return (0);
}

int comparePratial(std::string src, std::string find)
{
	int i = 0;

	for (; src[i] && find[i]; i++)
	{
		if (src[i] != find[i])
			return (i);
	}
	return (i);
}

std::string partialFind(std::map<std::string, Location> loc, std::string reqTarget)
{
	std::map<std::string, Location>::iterator itLoc = loc.begin();

	(void)reqTarget;
	for (; itLoc != loc.end(); itLoc++)
	{
		int i = comparePratial(itLoc->first, reqTarget);
		if (itLoc->first[i] == '/')
			return (itLoc->first);
	}
	return ("");
}

std::string absolutFind(std::map<std::string, Location> loc, std::string reqTarget)
{
	std::map<std::string, Location>::iterator itLoc = loc.find(reqTarget);
	if (itLoc == loc.end())
		return ("");
	return (itLoc->first);
}

std::pair<std::string, std::string> locFind(std::map<std::string, Location> loc, std::string reqTarget)
{
	if (reqTarget[reqTarget.size() - 1] == '/')
		reqTarget.erase(reqTarget.size() - 1);
	std::string	test = absolutFind(loc, reqTarget);
	if (test.empty())
		test = partialFind(loc, reqTarget);
	if (test.empty())
	{
		std::vector<std::string> splited = split(reqTarget, '/');
		if (splited.size() == 0)
			return(std::pair<std::string, std::string>(test, ""));
		std::string newTarget = reqTarget;
		int i = newTarget.size();

		newTarget.erase(i - splited[splited.size() - 1].size() - 1, i);

		test = absolutFind(loc, newTarget);
		if (test.empty())
			return (std::pair<std::string, std::string>(test, ""));
		std::cout << test << std::endl;
		return(std::pair<std::string, std::string>(test, splited[splited.size() - 1]));
	}
	return (std::pair<std::string, std::string>(test, ""));
}

Server &	getTargetServer(std::vector<std::pair<Server &, int> > sockets, int fdTarget)
{
	for (std::vector<std::pair<Server &, int> >::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		if ((*itS).second == fdTarget)
			return ((*itS).first);
	}
	return ((*sockets.begin()).first);
}

bool	isServerSocket(int fd, std::vector<socketServ> & sockets)
{
	for (std::vector<socketServ>::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		if ((*itS).servSock == fd)
			return (true);
	}
	return (false);
}
/*
	Returns socketServ reference that contains a specific socket, it doesn't
	take into account if it's a server or client socket
*/
socketServ &	getSocketServ(int targetFd, std::vector<socketServ> & sockets)
{
	for (std::vector<socketServ>::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		if ((*itS).servSock == targetFd)
			return (*itS);
		for (std::vector<int>::iterator itV = (*itS).clientSock.begin(); itV != (*itS).clientSock.end(); itV++)
		{
			if (*itV == targetFd)
				return (*itS);
		}
	}
	return (sockets.front());
}


void	addNewClient(int kq, int targetSock, std::vector<socketServ> & sockets)
{
	int							newClient;
	struct kevent				evSet;
	struct sockaddr_in			addrCl;
	socklen_t					addrLenCl = sizeof(addrCl);

	if ((newClient = accept(targetSock, (sockaddr *)&addrCl, &addrLenCl)) < 0)
		std::cerr << getTime() << RED BOLD "Accept: " << strerror(errno) << NC << std::endl;
	else
	{
		socketServ & tmpServ = getSocketServ(targetSock, sockets);
		tmpServ.clientSock.push_back(newClient);
		EV_SET(&evSet, newClient, EVFILT_READ, EV_ADD, 0, 0, NULL);
		kevent(kq, &evSet, 1, NULL, 0, NULL);
		std::cout << getTime() << GREEN BOLD "Client #" << newClient << " connected" NC << std::endl;
	}
}

void	cleanServer(int kq, std::vector<socketServ> & sockets)
{
	for (std::vector<socketServ>::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		std::vector<int>	cliVec = (*itS).clientSock;
		for (std::vector<int>::iterator itV = cliVec.begin(); itV != cliVec.end(); itV++)
		{
			close(*itV);
		}
		close((*itS).servSock);
	}
	if (kq > 0)
		close(kq);
}

void	disconnectClient(int kq, int fd, std::vector<socketServ> & sockets)
{
	struct kevent	evSet;

	for (std::vector<socketServ>::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		std::vector<int>	cliVec = (*itS).clientSock;
		for (std::vector<int>::iterator itV = cliVec.begin(); itV != cliVec.end(); itV++)
		{
			if (*itV == fd)
			{
				cliVec.erase(itV);
				break ;
			}
		}
	}
	EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, NULL ,0, NULL);
	kevent(kq, &evSet, 1, NULL, 0, NULL);
	close(fd);
	std::cout << getTime() << PURPLE BOLD "Client #" << fd << " disconnected" NC << std::endl;
}

int	readFromSocket(int clientSocket, std::map<int, mssg> & mssg)
{
	char		buffer[BUFFER_SIZE + 1];
	Request &	currentReq = mssg[clientSocket].req;

	int bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
	if (bytes_read < 0)
	{
		std::cerr << strerror(errno) << std::endl;
		return (1);
	}
	else if (bytes_read == 0)
		return (0);
	buffer[bytes_read] = '\0';
	currentReq.parseNewBuffer(buffer);
	std::cout << getTime() << BLUE BOLD "Reading data from client #" << clientSocket << "..." << NC << std::endl;
	return (0);
}

Response	generateResponse(Request & req, Server & serv)
{
	Response	res;

	std::string	str = "<h1>Hello " + req.getHeaderField()["user-agent"] + "</h1>";
	res.setBody(str);
	res.addHeaderField(std::pair<std::string, std::string>("content-length", toString(str.length())));
	(void)serv;
	return (res);
}


void	manageRequestState(mssg & message, int clientSocket, int kq, std::vector<socketServ> & sockets)
{
	struct kevent	evSet[2];

	if (message.req.getState() == __SUCCESFUL_PARSE__)
	{
		message.res = generateResponse(message.req, getSocketServ(clientSocket, sockets).serv);
		//Remove read event while we are waiting to write
		EV_SET(&evSet[0], clientSocket, EVFILT_READ, EV_DELETE, 0, 0, 0);
		EV_SET(&evSet[1], clientSocket, EVFILT_WRITE, EV_ADD, 0, 0, 0);
		kevent(kq, evSet, 2, 0, 0, 0);		
		message.req = Request();
	}
	else if (message.req.getState() == __UNSUCCESFUL_PARSE__)
	{
		std::cerr << getTime() << RED BOLD "Error parsing request" NC << std::endl;
		message.req = Request();
	}
}


void	runEventLoop(int kq, std::vector<socketServ> & sockets, size_t size)
{
	std::map<int, mssg>			mssg;
	struct kevent				evSet;
	std::vector<struct kevent>	evList(size);
	int							nbEvents;

	while (signaled)
	{
		if ((nbEvents = kevent(kq, NULL, 0, evList.data(), size, NULL)) < 0)
		{
			std::cerr << strerror(errno) << std::endl;
			signaled = false;
		}
		for (int i = 0; i < nbEvents; i++)
		{
			int	clientSocket = evList[i].ident;

			if (isServerSocket(clientSocket, sockets))
			{
				addNewClient(kq, clientSocket, sockets);
			}
			else if (evList[i].flags & EV_EOF)
			{
				disconnectClient(kq, clientSocket, sockets);
			}
			else if (evList[i].filter == EVFILT_READ)
			{
				if (readFromSocket(clientSocket, mssg) != 0)
				{
					disconnectClient(kq, clientSocket, sockets);
				}
				manageRequestState(mssg[clientSocket], clientSocket, kq, sockets);
			}
			else if (evList[i].filter == EVFILT_WRITE)
			{
				std::cout << getTime() << YELLOW BOLD "Sending data to client #" << clientSocket << "..." << NC << std::endl;
				send(clientSocket, mssg[clientSocket].res.generateResponse().data(), mssg[clientSocket].res.generateResponse().size(), 0);
				EV_SET(&evSet, clientSocket, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
				kevent(kq, &evSet, 1, 0, 0, 0);
				EV_SET(&evSet, clientSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
				kevent(kq, &evSet, 1, 0, 0, 0);
				mssg[clientSocket].res = Response();
			}

		}
	}
	cleanServer(kq, sockets);
}

void startServers(std::vector<Server> & s)
{
	int							kq;
	std::vector<socketServ>		sockets;
	std::vector<struct kevent>	evSet;

	std::cout << getTime() << BOLD GREEN "Starting servers..." NC << std::endl;
	sockets = initSockets(s);
	if ((kq = kqueue()) == -1)
	{
		throw std::runtime_error("Error creating kqueue()");
	}
	for (std::vector<socketServ>::iterator itS = sockets.begin(); itS != sockets.end(); itS++)
	{
		struct kevent	sEvent;
		
		EV_SET(&sEvent, (*itS).servSock, EVFILT_READ, EV_ADD, 0, 0, 0);
		evSet.push_back(sEvent);
	}
	if (kevent(kq, evSet.data(), evSet.size(), NULL, 0, NULL) == -1)
	{
		throw std::runtime_error("Error calling kevent()");
	}
	runEventLoop(kq, sockets, evSet.size());
}