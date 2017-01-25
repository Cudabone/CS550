#!/bin/bash
gcc -g -DRPC_SVC_FG -c -o p2p_clnt.o p2p_clnt.c
gcc -g -DRPC_SVC_FG -c -o p2p_client.o p2p_client.c
gcc -g -DRPC_SVC_FG -c -o p2p_xdr.o p2p_xdr.c
gcc -g -DRPC_SVC_FG -o p2p_client p2p_clnt.o p2p_client.o p2p_xdr.o
gcc -g -DRPC_SVC_FG -c -o p2p_svc.o p2p_svc.c
gcc -g -DRPC_SVC_FG -c -o p2p_server.o p2p_server.c
gcc -g -DRPC_SVC_FG -o p2p_server p2p_svc.o p2p_server.o p2p_xdr.o
