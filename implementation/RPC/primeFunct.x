/**
* Interface definition file
*/


/**
* NAME: OKUSANYA OLUWADAMILOLA
* COURSE: CSCI 512 - DISTRIBUTED SYSTEMS
* HOMEWORK: 2
* DATE: MONDAY OCT 5, 2015
*/
struct token {
	int numberOfHops;
	int initialTime;
	int numOfPrimes;
	unsigned int nextInt;
};

typedef struct token TOKEN;

program PRIMEFINDER{
	version ONE{
		TOKEN isPrime(TOKEN) = 1;
	} = 1;
} = 0x200000FF;

