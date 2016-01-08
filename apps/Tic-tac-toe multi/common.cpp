/* common.cpp
*
*  Synopsis: Contains constants and implementations for
*            socket helper functions.
*
*  Author: Okusanya Oluwadamilola
*
*  Date: 9/21/15
*/

#include "common.h"
#include <iomanip>
#include <sys/socket.h>

static const unsigned TYPE_LENGTH = 1;
static const unsigned SIZE_LENGTH = 4;

static const unsigned TIME_BUF_LENGTH = 20;

ssize_t socket_write(int fd, std::string data, char type)
{
    std::stringstream out_msg;
    out_msg << type;
    out_msg << std::setw(SIZE_LENGTH) << std::setfill('0') 
        << data.length();
    out_msg << data;

    return send(fd, out_msg.str().c_str(), out_msg.str().length(), 0);
}

Message socket_read(int fd)
{
    int result;
    Message msg;
    msg.type = 'D';
    msg.data = "";

    char length_buffer[SIZE_LENGTH + 1] = {0};
    unsigned message_length;

    char message_type;
    std::string message_data;

    result = recv(fd, &message_type, TYPE_LENGTH, 0);

    result = recv(fd, &length_buffer, SIZE_LENGTH, 0);
    message_length = atoi(length_buffer);

    if(message_length == 0) return msg;
    char* message_buffer = new char[message_length + 1];
    memset(message_buffer, 0, message_length + 1);

    result = recv(fd, message_buffer, message_length, 0);
    message_data = message_buffer;

    msg.type = message_type;
    msg.data = message_data;
    return msg;
}
