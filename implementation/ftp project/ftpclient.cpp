/**
* NAME: OKUSANYA OLUWADAMILOLA
* COURSE: CSCI 512
* DATE: OCT 21, 2015
* HOMEWORK: FTP Client doing a specific function: retrieve a file from
*	    a specified FTP server
*/

#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <stddef.h>
#include <sstream>
#include <stddef.h>
#include <cctype>
#include <vector>


// Check if the pointer is pointing to a digit
int isDigits (const char * s){
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

// Calculate FTP data port
int calcDataPort(char buf[]){
	std::string s = std::string (buf);
	std::stringstream os(s);
	std::vector <std::string> tokens, nice;
	std::string temp;

	while(os >> temp){
		tokens.push_back(temp);
	}
	
	std::string st = tokens[tokens.size() - 1];
	std::cout << st << std::endl;

	st.erase(st.begin());
	st.erase(st.end() -1);

	while (st.find(",", 0) != std::string::npos) { //does the string a comma in it?
   		 size_t  pos = st.find(",", 0); //store the position of the delimiter
   		 temp = st.substr(0, pos);      //get the token
   		 st.erase(0, pos + 1);          //erase it from the source 
   		 nice.push_back(temp);                //and put it into the array
 	 }

  	nice.push_back(st);
	std::string str = nice[nice.size() - 1];
	int p2 = std::stoi(nice[nice.size() - 1]);
	int p1 = std::stoi(nice[nice.size() - 2]);	
	int dPort = (p1 << 8) + p2;   // new Port = p1 x 256 + p2
	return dPort; 		
}

// Get the IPAddress (C-style)
std::string getAddr(char buf[]){
	const char delimiters[] = " ,()";
   	char *token[9];
	char *ctoken[7];
	int n=0, nn, nt;
	char *cp = strdup(buf);
	token[n] = strtok(cp, delimiters);
   	while (token[0] && n < 8)
		token[++n] = strtok(NULL, delimiters);
	for (nn = 0, nt =0 ; nn<=n && nt < 7; ++nn){
		if (isDigits(token[nn]) != 0){
			ctoken[nt++] = token[nn];	
		}
	}

	std::string s1 = ".";
	std::stringstream ssm;
	ssm << ctoken[1] << s1 << ctoken[2] << s1 << ctoken[3] << s1 << ctoken[4];
	std::string result = ssm.str();
	free(cp);
	return result;
}	


// Print the reply from the FTP server
void reply(int sockdescript, char buf[]){
	int numBytesLeft = 1024;
	int numRead = 0; 
	int i = 0;   
	numRead = recv(sockdescript,buf, numBytesLeft, 0);
	char *p = (&buf[0]);
	printf("%s", p);
	
}

// Send commands to the FTP server
void command_user(int sockdescript, std::string str1, 
		  std::string str2, char buf[], int len){

    char *command = new char[str2.length() + 1];
    printf("%s", str1.c_str()); 
    memset(buf, 0, sizeof(*buf) * len);
    strcpy(command, str2.c_str());
    send(sockdescript, command, strlen(command), 0);
    delete [] command;
    reply(sockdescript, buf);
    
}

// Retrieve file command
void retreive_file_command(int sockdescript, std::string str1, 
		  std::string str2, char buf[], char buf2[],
		  int len){
    command_user(sockdescript, str1, str2, buf, len);
    memcpy(buf2, buf, strlen(buf));
}


int main(int argc, char *argv[]){

   struct hostent *hp;
   struct in_addr ip_addr;
   char tmp[1024], buf[1024];
   struct sockaddr_in FAddrStruct; 
   struct sockaddr_in DAddrStruct;
   std::string command_info, command_string = " ";
   int tcpsd;  // TCP connection socket descriptor
   int ftpsd; // FTP socket descriptor
   int ftpdsd;	// FTP data socket descriptor
   int fd, n_char;

   /* Verify a "hostname" parameter was supplied */
   if (argc <1 || *argv[1] == '\0')
      exit(EXIT_FAILURE);

   // Verify that it is a correct hostname
   hp = gethostbyname(argv[1]);

   if (!hp) {
      printf("%s was not resolved\n",argv[1]);
      exit(EXIT_FAILURE);
   }
   
   // Print to the screen 

   ip_addr = *(struct in_addr *)(hp->h_addr);
   printf("Connecting to : %s\n server IP is : %s\n",
           argv[1],inet_ntoa(ip_addr));

   /*
    * Connection Setup begins
    */ 	
   // Create the TCP socket structure
   tcpsd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

   if (tcpsd < 0){
	perror("TCP socket creation: Failure \n");
	exit(EXIT_FAILURE);
   } else{
	printf("TCP socket creation: Success \n");	
   }
	

   // Construct address structures for both TCP server
   // and local data server
   memset(&FAddrStruct, 0, sizeof(FAddrStruct));
   FAddrStruct.sin_family = AF_INET;
   memcpy(&FAddrStruct.sin_addr, hp->h_addr, hp->h_length);
   FAddrStruct.sin_port = htons(21); // Default server port is 21.

   memset(&DAddrStruct, 0, sizeof(DAddrStruct));
   DAddrStruct.sin_family = AF_INET;
   DAddrStruct.sin_addr.s_addr = htonl(INADDR_ANY);
   // Arbitary number > 1240 since we are using a passive mode
   // Number that is chosen can not be one of the well-known ports
   DAddrStruct.sin_port = htons(1240);  

   ftpsd = connect(tcpsd, 
		(struct sockaddr*) &FAddrStruct, 
		sizeof(FAddrStruct));
   if (ftpsd < 0){
	perror("FTP server connection: Failure \n");
	exit(EXIT_FAILURE);		
     } else{
	printf("FTP server connection: Success \n");
     }
   
   // Used to receive the initial set-up message from the server.
   memset(tmp, 0, sizeof(*tmp) * 1024);
   reply(tcpsd, tmp);

   // To bind and listen on the data server, we will bind to a 
   // "zero" file descriptor, This will correspond to the INADDR_ANY we used 
   //  in the DAddrStruct definition while giving us a random port
   // to work with. Backlog is set in order for the socket 
   // acceptance to be spontaneous.
   int backlog;
   bind(ftpsd, (struct sockaddr*)&DAddrStruct, 
	sizeof(DAddrStruct));
   listen(ftpsd, backlog);

   /*
    * Connection setup ends
    */

    /*
     * Command mode begins 
     */
    
   command_info = "Sending user name: cs412110\n";
   command_string = "USER cs412110\r\n";
   command_user(tcpsd, command_info, command_string, tmp, 1024);

   command_info = "Sending user password: 2OItcaf4 {\n";
   command_string = "PASS 2OItcaf4 {\r\n";
   command_user(tcpsd, command_info, command_string, tmp, 1024);

   command_info = "Checking type of system\n";
   command_string = "SYST\r\n";
   command_user(tcpsd, command_info, command_string, tmp, 1024);

   // Mode is not neccessary as its default is stream

   command_info = "Sending the type of file to be transferred\n";
   command_string = "TYPE A\r\n";
   command_user(tcpsd, command_info, command_string, tmp, 1024);

   // Passive mode is set since we want to resolve firewall 
   // issues.In passive mode, the server will wait for the client to establish a connection with it rather than attempting to 
   // connect to a client-specified port. The server will respond with the address of the port it is listening on
   command_info = "Setting the mode of connection to passive\n";
   command_string = "PASV\r\n";
   command_user(tcpsd, command_info, command_string, tmp, 1024);
   
  // Tokenizing the response in the buffer to generate the port number and address
   std::string s = getAddr(tmp);
   printf("Address: %s \n", s.c_str());
   int dAddPort = calcDataPort(tmp);
   printf(" Data Port : %d \n", dAddPort);

   /* 
    * Connection setup begins 
    */   
   // Connect to the FTP server on the data socket
   ftpdsd = socket(PF_INET, SOCK_STREAM, 0);
   if (ftpdsd < 0)
	perror("Data Socket creation: failure");

   // Reset the dataServAddr structure
   memset(&DAddrStruct, 0, sizeof(DAddrStruct));
   DAddrStruct.sin_family = AF_INET;
   DAddrStruct.sin_port = htons(dAddPort);
   DAddrStruct.sin_addr.s_addr = inet_addr(s.c_str());
 
  fd = connect(ftpdsd, (struct sockaddr *)&DAddrStruct, 
		sizeof(DAddrStruct));
  if (fd < 0)
	printf("FTP data socket connection: Failure\n");
  else
	printf("FTP data socket connection: Success\n");

  /* 
    * Connection setup ends
    */

   /*
    *  Retrieving data
    */
  command_info = "Downloading file\n";
  command_string = "RETR /export/home/cs412/cs412110/Documents/HW3/testfile.txt\r\n" ;
  retreive_file_command(tcpsd, command_info, command_string,
		tmp, buf, 1024);	
   
   /*
    * Print to screen and close sockets
    */
  memset(tmp, 0, strlen(tmp));
  n_char = read(ftpdsd, tmp, 1024);
  printf("The content of the file gotten" 
		" from the FTP server is : \n");
  n_char = write(1, tmp, n_char);
  close(ftpdsd);
   
  command_info = "Closing the FTP connection\n";
  command_string = "QUIT \r\n";
  command_user(tcpsd, command_info, command_string,
  		tmp, 1024);	    
  close(tcpsd);
  exit(0); 

}
