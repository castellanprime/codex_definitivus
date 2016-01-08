/* client.cpp
*
*  Synopsis: Tic-tac-toe game client.
*            This program connects to an instance of serverC and enables
*            the user to play a game of tic-tac-toe against the computer
*            or another player.
*
*  Author: Okusanya David
*
*  Date: 9/21/15
*/

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "common.h"

int main(int argc, char** argv)
{
    int client_sock;
    struct sockaddr_in serv_adr = {0};
    struct hostent* host;
    
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0] << 
            " <server address>" << std::endl;
        return 1;
    }

    host = gethostbyname(argv[1]);

    if(host == (struct hostent*)NULL)
    {
        perror("gethostbyname() failed");
        return 2;
    }

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_port = htons(PORT);
    memcpy(&serv_adr.sin_addr, host->h_addr, host->h_length);

    client_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(client_sock < 0)
    {
        perror("Could not create socket");
        return 3;
    }

    if(connect(client_sock, (struct sockaddr*)&serv_adr, 
        sizeof(serv_adr)) < 0)
    {
        perror("Could not connect to server");
        return 4;
    }

    std::string input;
    Message msg;

    // TODO: Error handling! :)O
    for(;;)
    {
        msg = socket_read(client_sock);

        switch(msg.type)
        {
            case MESSAGE_DISPLAY:
                std::cout << msg.data << std::endl;
                break;
            case MESSAGE_PROMPT:
                std::cout << msg.data;
                input.clear();
                std::cin >> input;
                socket_write(client_sock, input);
                break;
            case MESSAGE_TERMINATE:
                close(client_sock);
                exit(0);
                break;
            default:
                std::cout << "Unrecognized message "
                    << "type received. Ignoring...";
                break;
        }
    }
    
    close(client_sock);
    return 0;
}
