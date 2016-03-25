// Distributed implementation of the Dining Philosophers problem
// using distributed algorithm for mutual exclusion.
//
// 
// 

#include <iostream>
#include <sstream>
#include <queue>
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
#include <time.h>

static const int kStartPort = 23193;
static const int kTotalProcesses = 5;

struct ProcessInfo
{
	int socket_descriptor;
	std::string address;
};

enum MessageType
{
	OK,
    ONE,  // Numbering for the forks
    TWO,
    THREE,
    FOUR,
    FIVE,
};

const char* message_text[] = {"OK", "FORK ONE",
	"FORK TWO", "FORK THREE", "FORK FOUR", "FORK FIVE"};

// Macro that may be used to convert a message type to a text
// string for output.
#define MESS_TEXT(x) ((x >= 0 && x < sizeof(message_text)) ? \
	message_text[x] : "Unknown message")

// Used to keep track of critical region and timestamp
struct MessageInfo
{
    MessageType CR_ID;
    int proc_num;
    int time;
    
    MessageInfo(MessageType i, int p, int t) : CR_ID(i), proc_num(p), time(t) {}
};

// Queues for the forks
std::queue<int> forkOneQ;
std::queue<int> forkTwoQ;
std::queue<int> forkThreeQ;
std::queue<int> forkFourQ;
std::queue<int> forkFiveQ;

std::vector<ProcessInfo> processes(kTotalProcesses);
struct pollfd ufds[kTotalProcesses] = {{0}};

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
		// IDs are not the master.
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
    std::stringstream msg, msg_temp;
    int msg_int;
    int t = time(0);
	msg << message << " " << t;
    msg_temp << message;
    msg_temp >> msg_int;
	
    std::stringstream msg_size;
    msg_size << std::setw(30) << std::setfill('0')
             << msg.str().length();
    
    send(processes[id].socket_descriptor,
        msg_size.str().c_str(), msg_size.str().length(), 0);
    send(processes[id].socket_descriptor,
		msg.str().c_str(), msg.str().length(), 0);
	
	std::cout << "   [MESSAGE SENT TO " << id << "] "
		<< MESS_TEXT(msg_int) << " " << t << std::endl;
	return 0;
}

// Send a message to the provided process with provided timestamp.
int send_message(int id, MessageType message, int t)
{
    std::stringstream msg, msg_temp;
    int msg_int;
	msg << message << " " << t;
    msg_temp << message;
    msg_temp >> msg_int;
	
    std::stringstream msg_size;
    msg_size << std::setw(30) << std::setfill('0')
             << msg.str().length();
    
    send(processes[id].socket_descriptor,
        msg_size.str().c_str(), msg_size.str().length(), 0);
    send(processes[id].socket_descriptor,
		msg.str().c_str(), msg.str().length(), 0);
	
	std::cout << "   [MESSAGE SENT TO " << id << "] "
		<< MESS_TEXT(msg_int) << " " << t <<std::endl;
	return 0;
}

// Retrieve a messsage from the provided socket descriptor.
MessageInfo receive_message(int sock_desc)
{
    char buf[30] = {0};
    char msg_length[30 + 1] = {0};
    recv(sock_desc, &msg_length, sizeof(msg_length) - 1, 0);
    
	int rec_length;
	std::stringstream parse_length(msg_length);
	parse_length >> rec_length;
    
	//char* data = new char[rec_length + 1];
	//memset(data, 0, rec_length + 1);
    int res = recv(sock_desc, buf, rec_length, 0);
    
	// The only time we should read zero bytes is if the
	// other end disconnected. We'll just exit the program.
	if(res <= 0) exit(1);

	int in_msg_int;
    int t;
	MessageType in_msg;
	std::stringstream msg_in(buf);
	msg_in >> in_msg_int >> t;
	in_msg = (MessageType)in_msg_int;

	std::cout << "   [MESSAGE RECEIVED FROM " << sd_to_id(sock_desc)
		<< "] " << MESS_TEXT(in_msg_int) << " " << t << std::endl;
    
    MessageInfo return_msg(in_msg, sd_to_id(sock_desc), t);
    
	return return_msg;
}

struct Philosopher
{
    int id;
    int leftFork;
    int rightFork;
    int forkWanted;
    int timeAskedFor;
    int numberAte;
    
    Philosopher (int i) : id(i)
    {
        leftFork     = -1;
        rightFork    = -1;
        forkWanted   = -1;
        timeAskedFor = -1;
        numberAte    = 0;
    }
    
    // Methods
    void getLeftFork();
    void getRightFork();
    void dropForks();
};

// Works towards grabbing first fork
void Philosopher::getLeftFork ()
{
    std::cout << "Philosopher " << id << " is asking for left fork." << std::endl;
    timeAskedFor = time(0);

    if (id == 0)
    {
        // Request fork
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, ONE, timeAskedFor);
            forkWanted = 1;
        }
    }
    else if (id == 4)
    {
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, FIVE, timeAskedFor);
            forkWanted = 5;
        }
    }
    else
    {
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, MessageType(id+1), timeAskedFor);
            forkWanted = id+1;
        }
    }
    
    // Accept own request
    std::cout << "Send OK to own request." << std::endl;
    std::cout << "   [MESSAGE SENT TO " << id << "] OK ";
    std::cout << time(0) << std::endl;
}

// Works towards grabbing second fork
void Philosopher::getRightFork ()
{
    std::cout << "Philosopher " << id << " is asking for right fork." << std::endl;
    timeAskedFor = time(0);

    if (id == 0)
    {
        // Request forks
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, FIVE, timeAskedFor);
            forkWanted = 5;
        }
    }
    else if (id == 4)
    {
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, FOUR, timeAskedFor);
            forkWanted = 4;
        }
    }
    else
    {
        for (int i = 0; i < kTotalProcesses; i++)
        {
            send_message(i, MessageType(id), timeAskedFor);
            forkWanted = id;
        }
    }
    
    // Accept own request
    std::cout << "Send OK to own request." << std::endl;
    std::cout << "   [MESSAGE SENT TO " << id << "] OK ";
    std::cout << time(0) << std::endl;
}

// Release all forks, and notify anyone waiting
void Philosopher::dropForks ()
{
    leftFork   = -1;
    rightFork  = -1;
    forkWanted = -1;
    timeAskedFor = -1;
    
    std::cout << "Philosopher " << id << " is dropping their forks." << std::endl;
    
    // Go through each queue and send OK to all waiting
    if (id == 0)
    {
        for (int i = 0; i < forkFiveQ.size(); i++)
        {
            send_message(forkFiveQ.front(), OK);
            forkFiveQ.pop();
        }
        for (int i = 0; i < forkOneQ.size(); i++)
        {
            send_message(forkOneQ.front(), OK);
            forkOneQ.pop();
        }
    }
    else if (id == 1)
    {
        for (int i = 0; i < forkOneQ.size(); i++)
        {
            send_message(forkOneQ.front(), OK);
            forkOneQ.pop();
        }
        for (int i = 0; i < forkTwoQ.size(); i++)
        {
            send_message(forkTwoQ.front(), OK);
            forkTwoQ.pop();
        }
    }
    else if (id == 2)
    {
        for (int i = 0; i < forkThreeQ.size(); i++)
        {
            send_message(forkThreeQ.front(), OK);
            forkThreeQ.pop();
        }
        for (int i = 0; i < forkTwoQ.size(); i++)
        {
            send_message(forkTwoQ.front(), OK);
            forkTwoQ.pop();
        }
    }
    else if (id == 3)
    {
        for (int i = 0; i < forkThreeQ.size(); i++)
        {
            send_message(forkThreeQ.front(), OK);
            forkThreeQ.pop();
        }
        for (int i = 0; i < forkFourQ.size(); i++)
        {
            send_message(forkFourQ.front(), OK);
            forkFourQ.pop();
        }
    }
    else if (id == 4)
    {
        for (int i = 0; i < forkFiveQ.size(); i++)
        {
            send_message(forkFiveQ.front(), OK);
            forkFiveQ.pop();
        }
        for (int i = 0; i < forkFourQ.size(); i++)
        {
            send_message(forkFourQ.front(), OK);
            forkFourQ.pop();
        }
    }
}

// Main execution
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
    
    srand(getpid());
	int id;
	std::stringstream id_str(argv[1]);
	id_str >> id;

	int port = kStartPort + id;
	int listen_sock = create_listen_sock(port);

	if(id > 0)
		do_startup_routine(id, listen_sock, argv[2]);
	else
		do_startup_routine(id, listen_sock);

	std::cout << "All processes connected." << std::endl;
    
    int ufds_index = 0;
	for(int i = 0; i < kTotalProcesses; i++)
	{
		ufds[ufds_index].fd = processes[i].socket_descriptor;
		ufds[ufds_index].events = POLLIN;
		ufds_index++;
	}
    
    // Implementation of the hungry philosopher problem
    std::vector<Philosopher> phils;
    for (int i = 0; i < kTotalProcesses; i++)
    {
        Philosopher temp(i);
        phils.push_back(temp);
    }
    
    // Philosopher variables
    int counterOK       = 0;
    bool asking         = false;
    time_t doneEating   = 0;
    time_t doneThinking = 0;
    
    // Start by thinking
    std::cout << "Philosopher " << id << " is now thinking." << std::endl;
    
    while (1)
    {
        // Thinking delay
        if (time(0) >= doneThinking)
        {
            doneThinking = 0;
        
            // Even numbered procs are lefties
            if (id % 2 == 0)
            {
                if (counterOK == 4 && phils[id].leftFork == -1) // OK to grab left
                {
                    std::cout << "Philosopher " << id << " picks up fork ";
                    std::cout << phils[id].forkWanted << std::endl;
                
                    phils[id].leftFork = phils[id].forkWanted;
                    phils[id].forkWanted = -1;
                    asking = false;
                }
                else if (counterOK == 8 && doneEating == 0) // OK to grab right
                {
                    std::cout << "Philosopher " << id << " picks up fork ";
                    std::cout << phils[id].forkWanted << std::endl;
            
                    phils[id].rightFork = phils[id].forkWanted;
                    phils[id].forkWanted = -1;
                    asking = false;
                }
            }
            else // Odd numbered procs are righties
            {
                if (counterOK == 4 && phils[id].rightFork == -1) // OK to grab right
                {
                    std::cout << "Philosopher " << id << " picks up fork ";
                    std::cout << phils[id].forkWanted << std::endl;
            
                    phils[id].rightFork = phils[id].forkWanted;
                    phils[id].forkWanted = -1;
                    asking = false;
                }
                else if (counterOK == 8 && doneEating == 0) // OK to grab left
                {
                    std::cout << "Philosopher " << id << " picks up fork ";
                    std::cout << phils[id].forkWanted << std::endl;
            
                    phils[id].leftFork = phils[id].forkWanted;
                    phils[id].forkWanted = -1;
                    asking = false;
                }
            }
    
            // Check to see if philosopher is not looking for a fork
            if (phils[id].forkWanted == -1)
            {
                if (id % 2 == 0)
                {
                    // If not asking, lefty asks for left first
                    if (phils[id].leftFork == -1 && !asking)
                    {
                        std::cout << "Philosopher " << id << " is now hungry." << std::endl;
                
                        phils[id].getLeftFork();
                        asking = true;
                    }
                    else if (phils[id].rightFork == -1 && !asking) // Lefty asks for right second
                    {
                        phils[id].getRightFork();
                        asking = true;
                    }
                    else if (doneEating == 0) // Start eating
                    {
                        int random = rand()%4 + 1;
                        
                        std::cout << "Philosopher " << id << " is now eating for ";
                        std::cout << "the " << phils[id].numberAte+1 << " time(s)." << std::endl;
                        std::cout << "Now eating for " << random << " seconds" << std::endl;
                        
                        doneEating = time(0) + random;
                    }
                }
                else
                {
                    // If not asking, righty asks for right first
                    if (phils[id].rightFork == -1 && !asking)
                    {
                        std::cout << "Philosopher " << id << " is now hungry." << std::endl;
                
                        phils[id].getRightFork();
                        asking = true;
                    }
                    else if (phils[id].leftFork == -1 && !asking) // Righty asks for left second
                    {
                        phils[id].getLeftFork();
                        asking = true;
                    }
                    else if (doneEating == 0) // Start eating
                    {
                        int random = rand()%4 + 1;
                    
                        std::cout << "Philosopher " << id << " is now eating for ";
                        std::cout << "the " << phils[id].numberAte+1 << " time(s)." << std::endl;
                        std::cout << "Now eating for " << random << " seconds" << std::endl;
                    
                        doneEating = time(0) + random;
                    }
                }
            }
        
            // Done eating
            if (phils[id].leftFork != -1 && phils[id].rightFork != -1 && time(0) >= doneEating)
            {
                int random = rand()%4 + 1;
            
                std::cout << "Philosopher " << id << " is done eating." << std::endl;
        
                phils[id].numberAte++;
                phils[id].dropForks();
                counterOK  = 0;
                asking     = false;
                doneEating = 0;
            
                // Thinking
                std::cout << "Philosopher " << id << " is now thinking." << std::endl;
                std::cout << "Now thinking for " << random << " seconds" << std::endl;
                
                doneThinking = time(0) + random;
            }
        }
        
        int rv = poll(ufds, kTotalProcesses, 100);
		if(rv == -1) // Error
			exit(1);

		for(int i = 0; i < kTotalProcesses; i++)
		{
			if(ufds[i].revents & POLLIN)
			{
				MessageInfo msg = receive_message(ufds[i].fd);
                
                // Check if the message was a OK message
                if (msg.CR_ID == OK)
                {
                    counterOK++;
                    continue;
                }
                
                // Check to see if the fork is one we're interested in
                if (phils[id].forkWanted != (int)(msg.CR_ID) && phils[id].rightFork != (int)(msg.CR_ID) && phils[id].leftFork != (int)(msg.CR_ID))
                {
                    std::cout << "Not interested in this fork. OK." << std::endl;
                    send_message(sd_to_id(ufds[i].fd), OK);
                }
                else if (phils[id].forkWanted == (int)(msg.CR_ID) || phils[id].rightFork == (int)(msg.CR_ID) || phils[id].leftFork == (int)(msg.CR_ID))
                { // We either have the fork or want it
                
                    // Check if we own the fork
                    if (phils[id].rightFork == (int)(msg.CR_ID) || phils[id].leftFork == (int)(msg.CR_ID))
                    {
                        std::cout << "I own this fork so queue." << std::endl;
                        
                        // Queue request
                        if ((int)(msg.CR_ID) == 1)
                            forkOneQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 2)
                            forkTwoQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 3)
                            forkThreeQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 4)
                            forkFourQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 5)
                            forkFiveQ.push(sd_to_id(ufds[i].fd));
                    }
                    else if (msg.time > phils[id].timeAskedFor)
                    { // I want this fork and our timestamp is first
                    
                        std::cout << "Fork wanted is same fork I want but I'm first, so queue." << std::endl;
                        
                        // Queue request
                        if ((int)(msg.CR_ID) == 1)
                            forkOneQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 2)
                            forkTwoQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 3)
                            forkThreeQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 4)
                            forkFourQ.push(sd_to_id(ufds[i].fd));
                        else if ((int)(msg.CR_ID) == 5)
                            forkFiveQ.push(sd_to_id(ufds[i].fd));
                    }
                    else if (msg.time == phils[id].timeAskedFor)
                    { // I want this fork and we asked for it at same time
                    
                        // Let larger process number go first
                        if (msg.proc_num > id)
                        {
                            std::cout << "Time asked for fork is same as mine." << std::endl;
                            std::cout << "Their ID " << msg.proc_num << " is bigger than mine." << std::endl;
                            std::cout << "So they can eat first." << std::endl;
                            
                            send_message(sd_to_id(ufds[i].fd), OK, phils[id].timeAskedFor);
                        }
                        else
                        {
                            std::cout << "Time asked for fork is same as mine." << std::endl;
                            std::cout << "Their ID " << msg.proc_num << " is smaller than mine." << std::endl;
                            std::cout << "So I can eat first. Queue request." << std::endl;
                            
                            // Queue request
                            if ((int)(msg.CR_ID) == 1)
                                forkOneQ.push(sd_to_id(ufds[i].fd));
                            else if ((int)(msg.CR_ID) == 2)
                                forkTwoQ.push(sd_to_id(ufds[i].fd));
                            else if ((int)(msg.CR_ID) == 3)
                                forkThreeQ.push(sd_to_id(ufds[i].fd));
                            else if ((int)(msg.CR_ID) == 4)
                                forkFourQ.push(sd_to_id(ufds[i].fd));
                            else if ((int)(msg.CR_ID) == 5)
                                forkFiveQ.push(sd_to_id(ufds[i].fd));
                        }
                    }
                    else // Same fork I want, but they asked first
                    {
                        std::cout << "Fork wanted is the same fork I want but they're first, so send ok." << std::endl;
                        send_message(sd_to_id(ufds[i].fd), OK, phils[id].timeAskedFor);
                    }
                }
			}
		}
    }
    
	return 0;
}
