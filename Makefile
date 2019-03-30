all: mirror_client\
	sender\
	receiver\

receiver: receiver.c 
	gcc -g3 -Wall receiver.c -o receiver

sender: sender.c
	gcc -g3 -Wall sender.c -o sender

mirror_functions.o: mirror_functions.c mirror_functions.h
	gcc -g3 -c mirror_functions.c

mirror_client.o: mirror_client.c
	gcc -g3 -c mirror_client.c

mirror_client: mirror_client.o mirror_functions.o
	gcc -Wall mirror_client.o mirror_functions.o -o mirror_client

clean:
	rm -f sender receiver mirror_functions.o mirror_client.o mirror_client 

files:	
	rm -rf common
	rm -rf mirror1
	rm -rf mirror2
	rm -rf mirror3