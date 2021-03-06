/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "primeFunct.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

TOKEN *
isprime_1(TOKEN *argp, CLIENT *clnt)
{
	static TOKEN clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, isPrime,
		(xdrproc_t) xdr_TOKEN, (caddr_t) argp,
		(xdrproc_t) xdr_TOKEN, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
