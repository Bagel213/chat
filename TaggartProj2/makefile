##################################
# makefile
# Philip Taggart
#
# makefile for messager_server
# and messenger_client.cpp
# executables.  Includes clean
# function.
##################################

CC=g++ 
CFLAGS=-g -Wall -std=c++11 -pthread -pedantic

all: messenger_server messenger_client #messenger_client_two messenger_client_three 
	
messenger_server: messenger_server.cpp server.h
	$(CC) $(CFLAGS) -o messenger_server messenger_server.cpp

messenger_client: messenger_client.cpp client.h
	$(CC) $(CFLAGS) -o messenger_client messenger_client.cpp

#messenger_client_two: messenger_client_two.cpp client.h
#	$(CC) $(CFLAGS) -o messenger_client_two messenger_client_two.cpp

#messenger_client_three: messenger_client_three.cpp client.h
#	$(CC) $(CFLAGS) -o messenger_client_three messenger_client_three.cpp





clean:
	rm -f messenger_server
	rm -f messenger_client
	rm -f *.o
	rm -f *~
	rm -f core