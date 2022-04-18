#include "pch.h"
#include "InfoServer.h"

bool InfoServer::respond_to_all_requests()
{
	sockaddr_in addr;
	bool is_recv = Socket.RecvFrom(buff, MAX_BUFF_SIZE, reinterpret_cast<SOCKADDR*>(&addr));
	if (!is_recv) return false;

	if (strcmp(buff, "DISCOVERY\0") == 0)
	{
		Socket.SendTo(addr, response_info.c_str(), response_info.length());
	}
}

InfoServer::InfoServer()
{
	buff = (char*)malloc(MAX_BUFF_SIZE);
	Socket.Bind(&INFO_PORT);
}

void InfoServer::add_tracker()
{
	response_info = std::to_string(port_no) + ":Default\n";
}

void InfoServer::tick()
{
	while (respond_to_all_requests())
	{
	};
}
