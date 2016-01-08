/*	serverC.cpp
 * 
 * 	Synopsis: This is the server application for tic-tac-toe.
 *            This program listens for connections from game clients
 *            and spawns serverG for every connection (1-player mode)
 *            or every pair of connections (2-player mode).
 *
 *  Author: Okusanya Oluwadamilola
 *
 *  Date: 9/21/15
 */

#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <signal.h>
#include "common.h"

using namespace std;

// Signal handler for monitoring the termination of child processes.
void child_exited(int signum)
{
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) != -1)
    {
        std::cout << "Child process with PID " 
            << pid << " terminated." << std::endl;
    }
}

int main (int argc, char** argv)
{
    bool two_players;
    int new_sock, server_sock, pid;
    struct sockaddr_in server_address, clnt_address;

    if(argc < 2)
    {
        std::cout << "Usage: " << argv[0] 
            << " <# of players (1 or 2)>" << std::endl;
        return 9;
    }

    two_players = (argv[1][0] == '2');

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);

    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Could not create socket");
        return 1;
    }
    if (bind(server_sock, (struct sockaddr*)&server_address, 
                sizeof(server_address)) < 0)
    {
        perror("Could not bind socket");
        return 2;
    }
    if (listen(server_sock, 5) < 0)
    {
        perror("Error listening");
        return 3;
    }

    // Set up a signal handler so we can write a log
    // message for each serverG that terminates.
    signal(SIGCHLD, child_exited);

    while (1)
    {
        int client_sockets[2] = {0};
        socklen_t clnt_len = sizeof(clnt_address);
        for(int i = 0; i < (two_players ? 2 : 1); i++)
        {			
            if(i == 1)
            {
                socket_write(client_sockets[0],
                    "Waiting for opponent to connect...");
            }

            client_sockets[i] = accept(server_sock,
                (struct sockaddr*)&clnt_address, &clnt_len);

            std::stringstream hello_message;
            hello_message << "Hello, Player " << (i + 1) 
                << "! You are " 
                << (i == 0 ? "'x'" : "'o'") 
                << "; your opponent is " 
                << (i != 0 ? "'x'" : "'o'")
                << ".";

            socket_write(client_sockets[i], hello_message.str());

            char address_buffer[INET_ADDRSTRLEN];
            const char* result = inet_ntop(AF_INET, &clnt_address.sin_addr,
                address_buffer, sizeof(address_buffer));

            std::cout << "Connection accepted from " 
                << address_buffer << "." << std::endl;
        }

        pid = fork();
        if (pid == 0)
        {
            std::stringstream client1_sd, client2_sd;
            client1_sd << client_sockets[0];
            client2_sd << client_sockets[1];

            int exec_res;
            if(!two_players)
            {
                exec_res = execl("./serverG", "serverG", "1",
                    client1_sd.str().c_str(), NULL);
            } else
            {
                exec_res = execl("./serverG", "serverG", "2",
                        client1_sd.str().c_str(),
                        client2_sd.str().c_str(), NULL);
            }

            if(exec_res < 0)
            {
                perror("Could not start serverG");
                return 4;
            }
        }
        else if (pid > 0)
        {
            cout << "Child process created with PID " 
                << pid << "." <<  endl;
        }
        else
        {
            perror("Could not fork new process");
            return 5;
        }
    }

    return 0;
}
