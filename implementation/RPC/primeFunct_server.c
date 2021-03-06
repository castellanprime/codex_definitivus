/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

/**
* NAME: OKUSANYA OLUWADAMILOLA
* DATE: MONDAY OCT 5, 2015
*/

#define IS_PRIME 1
#define IS_NOT_PRIME 0
#include <math.h>
#include "primeFunct.h"


// isprime_1_svc function
// The main function that the server executes
// @args: address of a token, client handle
// @return: address of a token
TOKEN *
isprime_1_svc(TOKEN *argp, struct svc_req *rqstp)
{
	static TOKEN  result;

	/*
	 * insert server code here
	 */

	unsigned int numUTest = argp->nextInt;
	int hopCount = argp->numberOfHops;
	int primeCount = argp->numOfPrimes;
	
	if (isPrime_helper_fnct(numUTest) == IS_PRIME)
		primeCount++;
	hopCount++;
	numUTest++;
	result.nextInt = numUTest;
	result.numOfPrimes = primeCount;
	result.numberOfHops = hopCount;
	return (&result);
}


// isPrime_helper_fnct 
// A helper function to determine if a number is prime
// @args: unsigned int
// @return : one of the defined boolean values
int isPrime_helper_fnct(unsigned int number){
	unsigned int sqrt_n = 
		(unsigned int)sqrt((double) number);
	unsigned int var;
	if (number < 4)
		return IS_PRIME;
	else if (number > 3)
		for (var = 2; var <= sqrt_n ; var++)
			if ((number % var) == 0)
				return IS_NOT_PRIME;
	return IS_PRIME; 
}
