/* common.h
*
*  Synopsis: Contains constants and declarations for 
*            socket helper functions.
*
*  Author: Okusanya Oluwadamilola
*
*  Date: 9/21/15
*/

#ifndef COMMON_H_
#define COMMON_H_

#include <sstream>
#include <string>

static const unsigned PORT = 9999;

static const char MESSAGE_DISPLAY = 'D';
static const char MESSAGE_PROMPT = 'P';
static const char MESSAGE_TERMINATE = 'X';

struct Message
{
    char type;
    std::string data;
};

// These two functions implement a primitive protocol;
// messages traveling in either direction are prepended 
// with a single-character message type and a 4-digit
// ASCII integer representing the message's length:
// D0003MSG, P0011ENTER MOVE:, etc...
ssize_t socket_write(int fd, std::string data = "", 
    char type = MESSAGE_DISPLAY);
Message socket_read(int fd);

#endif
