Project 2 README

Make
Ensure that messenger_server.cpp, server.h, messenger_client.cpp, client.h are all present in the same directory with the makefile.  Type make at the command prompt to compile the programs to the executables messenger_server and messenger_client respectively.

Messenger Server
To run the server type messenger_server <user_info_file> <server configuration file> at the command prompt.  If the two input files are present the server will run loading the users, their passwords, and their friends list into memory.  It will also read in the port number for which it is to bind.  The server is now hands off, but to exit use Ctrl-C, which will close the listening socket, inform all clients the server is shutting down, and save user info to the file passed in initially.

The server will display to the terminal clients that connect and disconnect along with their ip, how many users are logged in, and when a client uses ctrl-c to disconnect.

If the port passed in the configuration file, the server will display the system selected port that can be passed to the clients.

Messenger Client
To run the client type messenger_client <client configuration file>  at the command prompt.  The configuration file must be in the same directory as the executable.  The ip and port in the configuration file must match the ip and port of server.  

The client will then connect to the server and prompt the user to register, login, or exit.  If there is an error binding to the socket due to another client using the same ip and port, the client will exit.  

If register is selected, enter a username and password, and if it is available, you will be logged in automatically. If the username is taken, you will be prompted to try again.  If login is selected you will be prompted for user name and password.  If there is a mismatch in either of these you will be prompted again.  Selecting exit will cause the client to exit.

Once logged in your friends list will display, or a message stating you have no friends online or do not yet have friends will be printed to the terminal.  This is followed by the main chat menu.

To message an online friend type m <friend username> <message>.  When a message is received it will display <friend username> >> <message>.
To invite someone to be a friend type i <username> <message>.  
To accept and invitation type ia <username><message>
To logout type logout.  The startup menu will then be displayed.

Every time a friend logs in, logs out, or is added your friends list with your current online friends will display.

Assumptions
We can assume the client's configuration file has the correct IP and port information, which has to be manually set based on ifconfig on the servers machine and either the port in the server configuration file or the system assigned port if displayed on server startup.

All messages sent including any preamble information, ie m <username> message, will be at most 1024 bytes.

Citations
Much of the server code involving select and the client's connection code was previously coded by me for our CPD group project.

NOTE
** A user info file with three users, a clientConfig, and a serverConfig file are included in the .tar for use if needed **


