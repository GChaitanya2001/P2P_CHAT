p2p: 	peer.c
	gcc peer.c -o p2p -lpthread

clean:	
	rm p2p