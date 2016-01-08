/*	serverG.cpp
 * 
 * 	Synopsis: This is the game manager for tic-tac-toe.
 *            This program is invoked by serverC, which passes one or two
 *            socket descriptors as arguments.
 *            serverG contains the logic and state for a game of tic-tac-toe.
 *
 *  Author: Okusanya Oluwadamilola
 *
 *  Date: 9/21/15
 */

#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include "common.h"

using namespace std;

/* Global variables */
char gameboard[3][3] = {'1','2','3',
                        '4','5','6',
                        '7','8','9'};

int client_sockets[2] = {0};
int numPlayer;

/* Function prototypes */
void sendBoard(int);
int validateMove(int);
void makeUser1Move(int);
void makeUser2Move(int);
void makeCompMove();
bool findWinner();

/* get_move(int)
 *
 * Synopsis: Helper function to get and validate a move from a player.
 *
 * Parameters: int - socket descriptor to use for sending the prompt
 *             and receiving the user's move.
 */
int get_move(int sock_desc)
{
    int move = 0;
    bool move_invalid = false;

    socket_write(sock_desc, "Enter a move: ", MESSAGE_PROMPT);

    do
    {
        move = atoi(socket_read(sock_desc).data.c_str());
        move_invalid = (validateMove(move) == -1 || move < 1 || move > 9);

        if(move_invalid)
        {
            socket_write(sock_desc,
                "Moving to a taken space or move is out "
                "of bounds.  Try again: ", MESSAGE_PROMPT);
        }

    } while(move_invalid);

    return move;
}

/* send_winner_message(char)
 *
 * Synopsis: This method will send a message indicating 
 *           the winner to each client.
 *
 * Parameters: char - Winning player's symbol.
 */
int send_winner_message(char winner)
{
    stringstream win_message;
    win_message << winner << "'s won!";

    for(int i = 0; i < numPlayer; i++)
    {
        sendBoard(client_sockets[i]);
        socket_write(client_sockets[i], win_message.str());
        socket_write(client_sockets[i], "TERM", 
            MESSAGE_TERMINATE);
    }
    return 0;
};

/* send_tie_message(char)
*
* Synopsis: This method will send a message to each client indicating
*           the game was a tie.
*
* Parameters: None.
*/
int send_tie_message()
{
    for(int i = 0; i < numPlayer; i++)
    {
        sendBoard(client_sockets[i]);
        socket_write(client_sockets[i], "The game ends in a tie!");
        socket_write(client_sockets[i], "TERM", 
            MESSAGE_TERMINATE);
    }
    return 0;
};

int main(int argc, char *argv[])
{
    int move;
    int socket_descriptor1;
    int socket_descriptor2;

    // Check valid number of arguments
    if (argc < 3)
    {
        std::cout << "Must pass the # of players and socket descriptor(s) "
            << "as arguments." << std::endl;
        return 1;
    }
    
    // Grab the number of players from arg 0
    numPlayer = atoi(argv[1]);
    
    // Capture socket descriptors from remaining args
    for (int i = 1; i < argc-1; i++)
    {
        client_sockets[i-1] = atoi(argv[i + 1]);
    }
    
    for (int i = 0; i < 5; i++)
    {
        socket_write(client_sockets[1], 
            "Waiting for opponent to make a move...");

        // Send the board to player 1.
        sendBoard(client_sockets[0]);

        socket_write(client_sockets[0], "Your turn!");
        move = get_move(client_sockets[0]);
            
        // Make user move
        makeUser1Move(move);
        sendBoard(client_sockets[0]);
                
        if ((numPlayer == 1) && (i != 4))
        {
            // One player game; make computer move.
            makeCompMove();
        }
        else if ((numPlayer == 2) && (i != 4))
        {
            // Check winner
            if (findWinner()) return 0;

            socket_write(client_sockets[0], 
                "Waiting for opponent to make a move...");
            
            // Send the board to player 2.
            sendBoard(client_sockets[1]);

            socket_write(client_sockets[1], "Your turn!");
            move = get_move(client_sockets[1]);
            
            // Make user 2 move
            makeUser2Move(move);
            sendBoard(client_sockets[1]);
        }
        
        // Check winner
        if (findWinner()) return 0;
    }
    
    send_tie_message();    
    return 0;
}

/* sendBoard()
 *
 * Synopsis: This method will send a string stream that contains 
 * the current game board.
 *
 * Parameters: int - socket descriptor to send the gameboard to.
 */
void sendBoard(int sock_desc) {
    stringstream out;
    
    out << "\n " << gameboard[0][0] << " | " << gameboard[0][1] 
        << " | " << gameboard[0][2] << " \n";

    out << "-----------\n";

    out << " " << gameboard[1][0] << " | " << gameboard[1][1] 
        << " | " << gameboard[1][2] << " \n";

    out << "-----------\n";

    out << " " << gameboard[2][0] << " | " << gameboard[2][1] 
        << " | " << gameboard[2][2] << " \n";

    std::string output = out.str();
    socket_write(sock_desc, output);
}

/* validateMove(int)
 *
 * Synopsis: This method will take the "move" parameter and verify
 *  that there is no current x's or o's in that move box.
 *
 * Parameters: integer - this will be the move that is to be verified.
 */
int validateMove(int move) {
    switch (move)
    {
        case (1):
            if (gameboard[0][0] == 'x' || gameboard[0][0] == 'o')
            {
                return -1;
            }
            break;
        case (2):
            if (gameboard[0][1] == 'x' || gameboard[0][1] == 'o')
            {
                return -1;
            }
            break;
        case (3):
            if (gameboard[0][2] == 'x' || gameboard[0][2] == 'o')
            {
               return -1;
            }
            break;
        case (4):
            if (gameboard[1][0] == 'x' || gameboard[1][0] == 'o')
            {
                return -1;
            }
            break;
        case (5):
            if (gameboard[1][1] == 'x' || gameboard[1][1] == 'o')
            {
                return -1;
            }
            break;
        case (6):
            if (gameboard[1][2] == 'x' || gameboard[1][2] == 'o')
            {
                return -1;
            }
            break;
        case (7):
            if (gameboard[2][0] == 'x' || gameboard[2][0] == 'o')
            {
                return -1;
            }
            break;
        case (8):
            if (gameboard[2][1] == 'x' || gameboard[2][1] == 'o')
            {
                return -1;
            }
            break;
        case (9):
            if (gameboard[2][2] == 'x' || gameboard[2][2] == 'o')
            {
                return -1;
            }
            break;
    }
    return 1;
}

/* makeUser1Move(int)
 *
 * Synopsis: This method will fill the box on the gameboard corresponding 
 * to the move user 1 wishes to make.
 *
 * Parameters: integer - this will be the move that is to be made.
 */
void makeUser1Move(int move)
{
    switch (move)
    {
        case (1): gameboard[0][0] = 'x'; break;
        case (2): gameboard[0][1] = 'x'; break;
        case (3): gameboard[0][2] = 'x'; break;
        case (4): gameboard[1][0] = 'x'; break;
        case (5): gameboard[1][1] = 'x'; break;
        case (6): gameboard[1][2] = 'x'; break;
        case (7): gameboard[2][0] = 'x'; break;
        case (8): gameboard[2][1] = 'x'; break;
        case (9): gameboard[2][2] = 'x'; break;
    }
}

/* makeUser2Move(int)
 *
 * Synopsis: This method will fill the box on the gameboard 
 *			corresponding to the move user 2 wishes to make.
 *
 * Parameters: integer - this will be the move that is to be made.
 */
void makeUser2Move(int move)
{
    switch (move)
    {
        case (1): gameboard[0][0] = 'o'; break;
        case (2): gameboard[0][1] = 'o'; break;
        case (3): gameboard[0][2] = 'o'; break;
        case (4): gameboard[1][0] = 'o'; break;
        case (5): gameboard[1][1] = 'o'; break;
        case (6): gameboard[1][2] = 'o'; break;
        case (7): gameboard[2][0] = 'o'; break;
        case (8): gameboard[2][1] = 'o'; break;
        case (9): gameboard[2][2] = 'o'; break;
    }
}

/* makeCompMove()
 *
 * Synopsis: This method will fill the box on the gameboard corresponding
 *			 to the random move the computer generates.
 *
 * Parameters: NONE
 */
void makeCompMove()
{

    // Seed random number generator
    srand((unsigned)time(NULL));
   
     while (true)
     {
         // Generate number between 1 - 9
         int move = (rand()%(9-1)) + 1;
         
         if (validateMove(move) != -1)
         {
            switch (move)
            {
                case (1): gameboard[0][0] = 'o'; break;
                case (2): gameboard[0][1] = 'o'; break;
                case (3): gameboard[0][2] = 'o'; break;
                case (4): gameboard[1][0] = 'o'; break;
                case (5): gameboard[1][1] = 'o'; break;
                case (6): gameboard[1][2] = 'o'; break;
                case (7): gameboard[2][0] = 'o'; break;
                case (8): gameboard[2][1] = 'o'; break;
                case (9): gameboard[2][2] = 'o'; break;
            }
            break;
         }
    }
}

/* findWinner()
 *
 * Synopsis: This method will check the game board for three 
 *           x's or o's in a row.
 *           If found, this method will let the user(s) know who won.
 *
 * Parameters: NONE
 */
bool findWinner()
{
    int count;
    char last_char;
    
    // Check horizontal winner
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (j == 0)
            {
                last_char = gameboard[i][j];
                count     = 1;
                continue;
            }
            if (gameboard[i][j] == last_char)
            {
                count++;
            }
            if (j == 2)
            {
                if (count == 3)
                {
                    send_winner_message(last_char);
                    return true;
                }
            }
        }
    }
    
    // Check vertical winner
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (j == 0)
            {
                last_char = gameboard[j][i];
                count     = 1;
                continue;
            }
            if (gameboard[j][i] == last_char)
            {
                count++;
            }
            if (j == 2)
            {
                if (count == 3)
                {
                    send_winner_message(last_char);
                    return true;
                }
            }
        }
    }
    
    // Check diagonal winner
    last_char = gameboard[0][0];
    if (last_char == gameboard[1][1])
    {
        if (last_char == gameboard[2][2])
        {
            send_winner_message(last_char);
            return true;
        }
    }
    
    last_char = gameboard[0][2];
    if (last_char == gameboard[1][1])
    {
        if (last_char == gameboard[2][0])
        {
            send_winner_message(last_char);
            return true;
        }
    }
    
    return false;
}
