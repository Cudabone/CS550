/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <stdio.h>
#include "p2p.h"

int *
registry_1_svc(peerfile *argp, struct svc_req *rqstp)
{

	static int  result;

	printf("Registry function called\n");
	/*
	 * insert server code here
	 */

	return(&result);
}
