struct peerfile {
	int peerid;
	string filename<>;	
};
program PEER_PROG {
		version PEER_VER {
				int registry(peerfile) = 1;
		} = 1;
} = 0x20000000;
