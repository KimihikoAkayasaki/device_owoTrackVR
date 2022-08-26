#pragma once

#include "Network.h"
#include <vector>

class InfoServer {

	uint32_t INFO_PORT = 35903;
	uint32_t port_no = 6969;

	UDPSocket Socket;

	char* buff;
	const int MAX_BUFF_SIZE = 64;

	std::string response_info;

	bool respond_to_all_requests();

public:
	InfoServer(bool& _ret);

	void add_tracker();
	void tick();

	void set_port_no(uint32_t const& new_port_no)
	{
		port_no = new_port_no;
	}
};