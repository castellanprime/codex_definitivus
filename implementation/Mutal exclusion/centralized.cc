#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iomanip>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/poll.h>

static const int kStartPort = 23193;
static const int kTotalProcesses = 5;

struct ProcessInfo
{
	int socket_descriptor;
	std::string address;
};

std::vector<ProcessInfo> processes(kTotalProcesses);
int election_in_progress = false;
int coordinator_id = 0;

enum MessageType
{
	ELECTION,
	OK,
	COORDINATOR,
	REQUEST,
	RELEASE
};

// Create a socket that is to be used for listening for incoming connections.
// Returns the socket descriptor number.
int create_listen_sock(int port)
{
	int listen_sock;
	struct sockaddr_in listen_address = {0};

	listen_address.sin_family = AF_INET;
	listen_address.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_address.sin_port = htons(port);

	listen_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(listen_sock < 0)
	{
		perror("Could not create socket");
		exit(1);
	}

	if (bind(listen_sock, (struct sockaddr*)&listen_address, 
		sizeof(listen_address)) < 0)
	{
		perror("Could not bind socket");
		exit(1);
	}

	if (listen(listen_sock, 5) < 0)
	{
		perror("Error listening");
		exit(1);
	}

	return listen_sock;
}

// Accept an incoming connection. Returns the socket descriptor number.
int accept_connection(int listen_sock)
{
	int new_socket;
	int result;
	struct sockaddr_in clnt_address = {0};
	socklen_t clnt_len = sizeof(clnt_address);

	new_socket = accept(listen_sock, 
		(struct sockaddr*)&clnt_address, &clnt_len);

	char address_buffer[INET_ADDRSTRLEN] = {0};
	const char* ntop_result = inet_ntop(AF_INET, 
		&clnt_address.sin_addr, address_buffer, 
		sizeof(address_buffer));

	if(ntop_result == NULL)
	{
		perror("inet_ntop failed");
		exit(1);
	}

	char id_buffer[3 + 1] = {0};
	result = recv(new_socket, &id_buffer, sizeof(id_buffer) - 1, 0);

	int new_id;
	std::stringstream id_str(id_buffer);
	id_str >> new_id;

	processes[new_id].socket_descriptor = new_socket;
	processes[new_id].address = address_buffer;

	std::cout << "New connection from id " << new_id 
		<< " at address " << address_buffer << "." << std::endl;

	return new_socket;
}

// Connect to another process and send XX.
// Returns the socket descriptor number.
int connect_to_process(std::string address, int port)
{
	int int_sock;
	struct addrinfo hints = {0};
	struct addrinfo *res, *res0;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	std::stringstream port_str;
	port_str << port;

	if(getaddrinfo(address.c_str(), port_str.str().c_str(), &hints, &res0) != 0)
	{
		perror("getaddrinfo failed");
		exit(1);
	}

	for(res = res0; res; res = res->ai_next)
	{
		int_sock = socket(res->ai_family, 
			res->ai_socktype, res->ai_protocol);
		if(int_sock < 0) continue;

		if(connect(int_sock, res->ai_addr, res->ai_addrlen) < 0)
		{
			close(int_sock);
			continue;
		}

		freeaddrinfo(res0);
		return int_sock;
	}

	freeaddrinfo(res0);
	perror("Could not connect to server");
	exit(1);
	return int_sock;
}

// Get connections established between all of the nodes
// and populate the process info vector.
void do_startup_routine(int my_id, int listen_sock, std::string master_addr = "")
{
	// If we're not the "master" (initial listener), go ahead
	// and connect to the "master."
	if(my_id > 0)
	{
		int int_sock = connect_to_process(master_addr, kStartPort);
		processes[0].address = master_addr;
		processes[0].socket_descriptor = int_sock;

		char buff[3];
		sprintf(buff, "%03d", my_id);
		send(int_sock, buff, sizeof(buff), 0);
	}

	// TODO: Since this part blocks, select()/poll() may be needed here...
	// "Master" waits for (N - 1) connections; others wait for (ID - 1).
	int num_incoming_conns = (my_id > 0) ? 
		(my_id - 1) : (kTotalProcesses - 1);
	for(int i = 0; i < num_incoming_conns; i++)
		accept_connection(listen_sock);

	// If we're the "master," we'll send the info we have about the
	// other processes to all the other processes.	
	if (my_id == 0)
	{
		std::stringstream proc_info;
    
		for (int i = 1; i < processes.size(); i++)
			proc_info << i << " " << processes[i].address 
				<< std::endl;
    
		std::stringstream msg_size;
		msg_size << std::setw(4) << std::setfill('0') 
			<< proc_info.str().length();
    
		for (int i = 1; i < kTotalProcesses; i++)
		{
		    send(processes[i].socket_descriptor, 
			msg_size.str().c_str(), msg_size.str().length(), 0);
		    send(processes[i].socket_descriptor, 
			proc_info.str().c_str(), proc_info.str().length(), 0);
		}
	}

	// If we're NOT the "master," we skipped the above and will
	// wait for the process info from the "master."
	if (my_id > 0)
	{
		char msg_length[4 + 1] = {0};
		recv(processes[0].socket_descriptor, 
			&msg_length, sizeof(msg_length) - 1, 0);
    
		int rec_length;
		std::stringstream parse_length(msg_length);
		parse_length >> rec_length;
    
		char* data = new char[rec_length + 1];
		memset(data, 0, rec_length + 1);
		recv(processes[0].socket_descriptor, data, rec_length, 0);
		std::stringstream item_parser(data);
		std::string item_address;
		int item_id;
		while(item_parser >> item_id && item_parser >> item_address)
			processes[item_id].address = item_address;
		delete data;

		// Now we have to connect to all the processes whose
		// IDs are higher than ours.
		for(int i = my_id + 1; i < kTotalProcesses; i++)
		{
			int int_sock = connect_to_process
				(processes[i].address, kStartPort + i);
			processes[i].socket_descriptor = int_sock;

			char buff[3];
			sprintf(buff, "%03d", my_id);
			send(int_sock, buff, sizeof(buff), 0);
		}
	}
}

// Get an ID from a socket descriptor number.
int sd_to_id(int socket_desc)
{
	for(int i = 0; i < kTotalProcesses; i++)
	{
		if(processes[i].socket_descriptor == socket_desc)
			return i;
	}
	return -1;
}

// Send a message to the provided process.
int send_message(int id, MessageType message)
{
	std::stringstream msg;
	msg << message;
	send(processes[id].socket_descriptor, 
		msg.str().c_str(), msg.str().length(), 0);

	std::cout << "MESSAGE SENT TO " << id << ": " 
		<< message << std::endl;
	return 0;
}

void hold_election(int id)
{
	election_in_progress = true;
	for(int i = id + 1; i < kTotalProcesses; i++)
		send_message(i, ELECTION);

	// Now wait for OKs to come back. If no OKs in
	// ~3 seconds (?), we are coordinator.
	// If OKs come back, do nothing. They will
	// start an election. (but still within this
	// function)...
}

// Retrieve a messsage from the provided socket descriptor.
MessageType receive_message(int sock_desc)
{
	char buf[3] = {0};
	int res = recv(sock_desc, buf, sizeof(buf) - 1, 0);

	// The only time we should read zero bytes is if the
	// other end disconnected. We'll just exit the program.
	if(res <= 0) exit(1);

	int in_msg_int;
	MessageType in_msg;
	std::stringstream msg_in(buf);
	msg_in >> in_msg_int;
	in_msg = (MessageType)in_msg_int;

	std::cout << "MESSAGE RECEIVED FROM " << sd_to_id(sock_desc) 
		<< ": " << in_msg_int << std::endl;
	return in_msg;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "ID not specified. See documentation."
			<< std::endl << std::endl;
		return 1;
	}

	if(argc > 3)
	{
		std::cout << "Too many arguments. See documentation." 
			<< std::endl << std::endl;
		return 1;
	}

	int id;
	std::stringstream id_str(argv[1]);
	id_str >> id;

	int port = kStartPort + id;
	int listen_sock = create_listen_sock(port);

	if(id > 0)
		do_startup_routine(id, listen_sock, argv[2]);
	else
		do_startup_routine(id, listen_sock);

	std::cout << "All processes connected. Coordinator is initially "
		<< "process " << coordinator_id << "." << std::endl;

	// Connections are complete.
	if(id == 0) hold_election(0);

	int ufds_index = 0;
	struct pollfd ufds[kTotalProcesses - 1] = {{0}};
	for(int i = 0; i < kTotalProcesses; i++)
	{
		if(i == id) continue;
		ufds[ufds_index].fd = processes[i].socket_descriptor;
		ufds[ufds_index].events = POLLIN;
		ufds_index++;
	}

	for(;;)
	{
		int rv = poll(ufds, kTotalProcesses - 1, 3500);
		if(rv == -1)
			exit(1);

		for(int i = 0; i < (kTotalProcesses - 1); i++)
		{
			if(ufds[i].revents & POLLIN)
			{
				MessageType msg = receive_message(ufds[i].fd);
				/*
				if(msg == ELECTION)
				{
					send_message(sd_to_id(ufds[i].fd), OK);
					hold_election(id);
				}

				if(msg == OK)
				{
					election_in_progress = false;
				}
				if(election_in_progress) std::cout << "I AM COORDINATOR!" << std::endl;
				*/
			}
		}
	}

	return 0;
}

