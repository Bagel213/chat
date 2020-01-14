#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <bits/stdc++.h>
#include <string>
#include <cstring>
#include "client.h"
#include <thread>
#include <mutex>
#include <pthread.h>
using namespace std;

// Global Variables
string server_ip;
int server_port;
bool logout;
int sd;
map<string, User*> userList;
vector<string> friends;
string my_port;
string my_ip;
string uname;
mutex lo_lock;
mutex print_lock;
mutex invite_lock;
mutex message_lock;
mutex ul_lock;
int invite_flag = 0;
int repeatFlag = 0;
string inviteUname;
vector<pthread_t> threads;
int cfd;
map<string, int> chat_sd;
bool disconnect = 0;
bool logged_in = 0;
struct toThread{
		string pyld;
		string input;
	};
vector<int> all_sockets;
mutex sock_lock;
vector<string> online;


/////////////////////////////////////////////////////// Friend Object Funcs ////////////////////////////////////////////////////


void User::my_friend(){
	is_friend = 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////// Signal handler / cleanup /////////////////////////////////////////////////////



void sig_handler(int s){
	//cout << "sig handle" << endl;
	string input, pyld;
	//cout << "logged in " << logged_in << endl;
	
	lo_lock.lock();
	logout = 1;
	lo_lock.unlock();
	pyld = payload(input, "none");
	disconnect = 1;
	
	if (logged_in == 1){
		cout << "here"<< endl;
		lo_lock.lock();
			logout = 1; // logout from server
			lo_lock.unlock();
			input = "disconnect";
			pyld = payload(input, "none");
			send(sd, pyld.c_str(),strlen(pyld.c_str())+1,0); // Tell server that I am disconnecting
	}
	
	else{
		cout << endl; // otherwise just exit
		exit(0);
	}
}	


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////// Parse Config / Header formatting//////////////////////////////////////////////


void configParse(string cfile){
	
	vector<string> config;
    string line;
	ifstream config_file(cfile);
			
	 // Open and read configuration
    if (config_file.is_open()){
        while(config_file >> line)
        	config.push_back(line);
    } 
    else cout << "configuration_file failed to open";
    config_file.close();
    server_ip = config[1];
    server_port = stoi(config[3]); 
    //cout << server_ip << " " << server_port<<endl; 
}

// add standard info to
string payload(string input, string pload){

	string payld = input + "|" + uname + "|" + my_ip + "|" + my_port + "|" + pload;
	return payld;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////// Connect to Server and Listen for connections ///////////////////////////////////////


void connect_to_server(){
	char* connected = new char[9];
	sd = 0; //socket descriptor
	struct sockaddr_in srv_addr; // struct for server IP and port numbers
	int len;

	// Create file descriptor for the socket
	if ((sd=socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed.  Ending.\n");
		exit(EXIT_FAILURE);
	}


	// Set up server_addr struct
	bzero(&srv_addr, sizeof(srv_addr)); // Zero out all fields
	srv_addr.sin_family = AF_INET;     // Set to IPv4
	srv_addr.sin_port = htons(server_port);    // Set port to port read from config, htons converts from host to network byte order


	// Add server ip to struct.  inet_pton turns string to binary
	inet_pton(AF_INET, server_ip.c_str(), &srv_addr.sin_addr);
	
	
	// Attempt to connect to server
	if (connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) 
	{ 
		perror("Socket failed.  Ending \n");
		exit(EXIT_FAILURE); 
	}

	sock_lock.lock();
	all_sockets.push_back(sd); // add to socket vector
	sock_lock.unlock();

	if (read(sd, connected, 10)==0){
		//cout << "connection failed." << endl;
		cout << "\nGoodbye!\n" << endl;
		exit(0); 
	}
	else
		cout << "Status = " << connected << endl << endl;
	
	// Get self IP
	len = sizeof(srv_addr);
	getsockname(sd, (struct sockaddr *) &srv_addr, (socklen_t *)&len);
	my_ip = inet_ntoa(srv_addr.sin_addr);
	//cout << my_ip << endl;
}


// Listen for friends trying to connect
void *chat_listen(void *arg){
	pthread_t tempTh;
	int opt=1; // used in setting options for socket
	struct sockaddr_in cl_addr; // struct for server IP and port numbers
	int temp_socket;
	//my_port = "45000"; // For testing

	 // Create file descriptor for the socket
	if ((cfd=socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed.\n");
		exit(EXIT_FAILURE);
	}


	// Set up srvaddr struct
	bzero(&cl_addr, sizeof(cl_addr)); // Zero out all fields
	cl_addr.sin_family = AF_INET;     // Set to IPv4
	cl_addr.sin_port = htons(stoi(my_port));    // Set port to config port number, htons converts from host to network byte order
	cl_addr.sin_addr.s_addr = INADDR_ANY; // Allow any address to connect


	// Set socket level options to allow local reuse of addr and port
	if (setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt failed.  Ending\n");
		exit(EXIT_FAILURE);
	}


	// Bind the socket to our desired port
	if (bind(cfd, (struct sockaddr *)&cl_addr, sizeof(cl_addr))<0){
		perror("bind failed.  Ending\n");
		exit(EXIT_FAILURE);
	}


	// Set socket to accept incoming connections with queue size of 5
	if(listen(cfd, 5)<0){
		perror("listen failed.  Ending\n");
		return NULL;
	}
	
	sock_lock.lock();
	all_sockets.push_back(cfd); // add to socket vector
	sock_lock.unlock();
	
	int addrlen = sizeof(cl_addr);
	// Accept incoming connections
	for (;;){
		if ((temp_socket = accept(cfd, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen))<0){    
	        cout << temp_socket << endl;
	        return NULL;   
	    }  
	    else{
	  	// Create threads for new chats
	    pthread_create(&tempTh, NULL, friend_chat_incoming, (void *)&temp_socket);
		threads.push_back(tempTh);
		}
	}	
            	

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////// Chat Functions /////////////////////////////////////////////////////////


void *main_chat(void *arg){
	string input, temp;
	string pyld;
	int pos, fsd;
	string chat_uname, invite_uname;
	pthread_t tempTh;

	toThread *t = new toThread();

	
	cout << "-- Type \"m < <username> <message>\" to chat with online friend." << endl;
	cout << "-- Type \"i <username> <message>\" to invite a new friend." << endl;
	cout << "-- Type \"logout\" to log out of your account and return to the main menu.\n" << endl;
	
	// Getline loop for all user input
	while(disconnect!=1){
		getline (cin,input);
		//cout << "getline " << input << " " << input.size() << endl;
		
		// if logout
		if (input == "logout"){
			lo_lock.lock();
			logout = 1;
			lo_lock.unlock();
			input = "logoff";
			pyld = payload(input, "none");
			send(sd, pyld.c_str(),strlen(pyld.c_str())+1,0); 
			pthread_exit(NULL);
		}
		
		// invitation send
		else if (input[0] == 'i' && input[1] != 'a'){
			temp = build(input);
			pyld = payload("i",temp);
			send(sd, pyld.c_str(),strlen(pyld.c_str())+1,0);
		}

		// invitation accept
		invite_lock.lock();
		if (input.substr(0,2) == "ia" && invite_flag == 1){
			//cout << "accept" << endl;
			temp = input;
			pos = temp.find_first_of(" ");
			temp.erase(0,pos+1);
			pos = temp.find_first_of(" ");
			invite_uname = temp.substr(0,pos);
			temp.erase(0,pos+1);
			invite_uname += ";" + temp;
			invite_flag = 0;
			pyld = payload("ia", invite_uname);
			send(sd, pyld.c_str(),strlen(pyld.c_str())+1,0);
		}
		invite_lock.unlock();

		// send a message to a friend
		if (input[0] == 'm'){
			temp = input;
			pos = temp.find_first_of(" ");
			temp.erase(0,pos+1);
			pos = temp.find_first_of(" ");
			chat_uname = temp.substr(0,pos);
			temp.erase(0,pos+1);
			//cout << temp << endl;
			
			// send to friend already chatting with
			if (chat_sd.find(chat_uname)!=chat_sd.end()){
				fsd = chat_sd.at(chat_uname);
				pyld = uname + ">>" + temp;
				send(fsd, pyld.c_str(), strlen(pyld.c_str())+1,0);
			}
			
			// new thread for new connection to friend
			else{
				temp = input;
				pos = temp.find_first_of(" ");
				temp.erase(0,pos+1);
				pos = temp.find_first_of(" ");
				chat_uname = temp.substr(0,pos);
				temp.erase(0,pos+1);
				pyld = uname + ">>" + temp;
				t->input = input; t->pyld = pyld;
				pthread_create(&tempTh, NULL, friend_chat_outgoing, (void *)t);
				threads.push_back(tempTh);
     
			}
		}





	}
	return NULL;
}

// connect to friend via their ip and port
void *friend_chat_outgoing(void *t){
	//cout << "fr chat" << endl;
	struct toThread *my_t = (struct toThread*)t;
	string pyld = my_t->pyld;
	string input =  my_t->input;
	size_t pos;
	string friend_uname;
	User *user;
	int fsd;
	int messageNum = 0;
	string temp;

	pos = input.find_first_of(" ");
	input.erase(0,pos+1);
	pos = input.find_first_of(" ");
	friend_uname = input.substr(0,pos);
	
	if (userList.find(friend_uname) == userList.end())
		cout << friend_uname << " is not online." << endl;
	else{

		user = userList.at(friend_uname);
		int friend_port = stoi(user->port);
		string friend_ip = user->ip;
		char connected[1024];
		struct sockaddr_in fr_addr; // struct for server IP and port numbers

		// Create file descriptor for the socket
		if ((fsd=socket(AF_INET, SOCK_STREAM, 0)) == 0){
			perror("Socket failed.  Ending.\n");
			exit(EXIT_FAILURE);
		}


		// Set up server_addr struct
		bzero(&fr_addr, sizeof(fr_addr)); // Zero out all fields
		fr_addr.sin_family = AF_INET;     // Set to IPv4
		fr_addr.sin_port = htons(friend_port);    // Set port to port read from config, htons converts from host to network byte order


		// Add server ip to struct.  inet_pton turns string to binary
		inet_pton(AF_INET, friend_ip.c_str(), &fr_addr.sin_addr);
		
		
		// Attempt to connect to server
		if (connect(fsd, (struct sockaddr *)&fr_addr, sizeof(fr_addr)) < 0) 
		{ 
			perror("Socket failed.  Ending \n");
			exit(EXIT_FAILURE); 
		}

		sock_lock.lock();
		all_sockets.push_back(fsd); // add to socket vector
		sock_lock.unlock();
		
		while(1){
			if (read(fsd, connected, 1024)==0){
				//cout << "connection failed." << endl;
				return NULL; 
			}
			else{
				
				// if first message save on going chat info in map and send message
				if (messageNum == 0){
					//cout << "out" << endl;
					temp = connected;
					chat_sd.insert(pair<string,int>(temp,fsd)); 
					send(fsd, pyld.c_str(),strlen(pyld.c_str())+1,0);
					messageNum++;
				}
				// output incoming message
				else{
					cout << connected << endl;
				}
			}
		}
	}
	return NULL;
}

// Read chat after friend connection accepted
void *friend_chat_incoming(void *temp_socket){
	int fsd = *(int*)temp_socket;
	
	char incoming[1024];
	string con = uname;
	int messageNum = 0;
	string temp;

	send(fsd, con.c_str(), strlen(con.c_str())+1,0);
	
	while(1){
		if (read(fsd, incoming, 1024)==0){
			//cout << "new chat connection with failed." << endl;
			return NULL; 
		}
		else{
			if (messageNum == 0){
				//cout << "in" << endl;
				temp = incoming;
				chat_sd.insert(pair<string,int>(temp,fsd));
				messageNum++;
				cout << incoming << endl;
			}
			else{
				cout << incoming << endl;
			}
		}
	}
	return NULL;

}

// formatting
string build(string input){

	size_t pos;
	string build;
	//cout << input << endl;
	pos = input.find_first_of(" ");
	input.erase(0,pos+1);
	pos = input.find_first_of(" ");
	build += input.substr(0,pos) + "|";
	input.erase(0,pos+1);
	build += input;
	return build;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////Manage User and Friends Updates /////////////////////////////////////////////////

// send invite to other user
void invite(string temp){
	size_t tokenPos;
    vector<string> tmp;

    tokenPos = temp.find("|"); 
    // Parse out username, ip, and port data
    while (tokenPos != string::npos){
    	tmp.push_back(temp.substr(0, tokenPos));
    	temp.erase(0, tokenPos+1);
    	tokenPos = temp.find("|");
    } 
    tmp.push_back(temp.substr(0, tokenPos));	
    cout << "User " << tmp[0] << " sent you and invite with the message: " << endl;
    cout << "\"" << tmp[1] << "\"" << endl;
    cout << "Type ia <username> <message> to accept." << endl;
	invite_lock.lock();
	invite_flag = 1;
	invite_lock.unlock();
}

// show another user logged off
void logoff_user(string temp){
	
	vector<string> userInfo;
	size_t tokenPos;
	
	tokenPos = temp.find(";"); 
    	
    // Parse out username, ip, and port data
    while (tokenPos != string::npos){
    	userInfo.push_back(temp.substr(0, tokenPos));
    	temp.erase(0, tokenPos+1);
    	tokenPos = temp.find(";");
    } 
    if (userList.find(userInfo[0])!=userList.end())
    	cout << "\n" << userInfo[0] << " logged off.\n" << endl;
    ul_lock.lock();
    userList.erase(userInfo[0]);
    ul_lock.unlock();
    print_online_friends();
}


// Print online friends
void print_online_friends(){
	
	size_t j;
	string temp;
	vector<string> old;
	
	old = online;
	online.clear();

	for(auto i = userList.begin(); i!=userList.end(); ++i){	
		
		// Build list of friends online by comparing 
		// friends list against online users
		for (j = 0; j<friends.size(); ++j){
			
			temp = i->second->username;
			friends[j].erase(friends[j].find_last_not_of(" \n\r\t")+1); // Trim trailing whitespace
			
			if (friends[j] == temp){
				i->second->my_friend();
				if (find(online.begin(), online.end(), friends[j]) == online.end())
					online.push_back(friends[j]);
			}
		}
	}
	//cout << online.size() << " " << old.size() << endl;
	if (online.size()!=0 && online.size() != old.size()){
		cout << "You have friends online: ";
		for (j = 0; j<online.size(); ++j)
			cout << online[j] << " ";
		cout << endl;
		repeatFlag = 0;
	}
	
	// Print online friends or none
	else if (online.size() == 0 && repeatFlag == 0){
		cout << "You currently have no friends online." << endl;
		repeatFlag = 1;
	}
	
	old.clear();

}

//New Login update
void new_login(string temp){
	
	vector<string> userInfo;
	User *user;	
	size_t tokenPos;
	tokenPos = temp.find(";"); 
    	
    // Parse out username, ip, and port data
    while (tokenPos != string::npos){
    	userInfo.push_back(temp.substr(0, tokenPos));
    	temp.erase(0, tokenPos+1);
    	tokenPos = temp.find(";");
    } 
     userInfo.push_back(temp.substr(0, tokenPos));	
    
    // build new user object and store in a map with username as key
    user = new(User);
	user->username = userInfo[0];
	user->ip = userInfo[1];
	user->port = userInfo[2];
	ul_lock.lock();
	userList.insert(pair<string,User*>(user->username,user));
	ul_lock.unlock();
	user = NULL;
    userInfo.clear();
    //cout << "\n ** " << userInfo[0] << " is online. **\n" << endl;
    print_online_friends();
}


// Build initial online user list
void get_online_users(string userString){
	
	vector<string> userInfo, userV;
	User *user;	
	string temp;
	size_t tokenPos;
	
	// Parse
	tokenPos = userString.find("|"); 
    while (tokenPos != string::npos){
    	userV.push_back(userString.substr(0, tokenPos));
    	userString.erase(0, tokenPos+1);
    	tokenPos = userString.find("|"); 
    }

    // Iterate through all obtained users
    for (size_t i = 0; i < userV.size(); i++){
     	
     	temp=userV[i];
     	tokenPos = temp.find(";"); 
    	
     	// Parse out username, ip, and port data
    	while (tokenPos != string::npos){
    		userInfo.push_back(temp.substr(0, tokenPos));
    		temp.erase(0, tokenPos+1);
    		tokenPos = temp.find(";");
    	} 
    	
    	// build new user object and store in a map with username as key
    	userInfo.push_back(temp.substr(0, tokenPos));
    	//cout << userInfo[0] << " " << userInfo[1] << " " << userInfo[2] << " " << endl;
    	user = new(User);
	   	user->username = userInfo[0];
	   	user->ip = userInfo[1];
	   	user->port = userInfo[2];
	   	ul_lock.lock();
	   	if (userList.find(user->username)==userList.end())
	   			userList.insert(pair<string,User*>(user->username,user));
	   	ul_lock.unlock();
	   	user = NULL;
    	userInfo.clear();
    }
}


// Build List of all friends
void get_friends_list(string friendString){
	
	string temp = friendString;
	size_t tokenPos;
	vector<string>::iterator it;
	
	// Parse
	tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
    	friends.push_back(temp.substr(0, tokenPos));
    	temp.erase(0, tokenPos+1);
    	tokenPos = temp.find("|"); 
    }
    temp.erase(0, tokenPos+1);
    
    // Add to list of friends and print
    friends.push_back(temp.substr(0, tokenPos));
    print_online_friends();
}

// Loop to wait for updates
void update_users(){
	
	char buf[1024];
	string temp, tmp;
    size_t tokenPos;
        
	// loop for information sent from server
	while(!logout){
		

		// Read from server connection
		if (read(sd, buf, 1024)==0){
			//cout << "connection failed." << endl;
			disconnect = 1;
			logout = 1;
			return;
		}
		
		// Parse "header"
		temp = buf;
		tokenPos = temp.find("&"); 
    	while (tokenPos != string::npos){
        	tmp = (temp.substr(0, tokenPos));
        	temp.erase(0, tokenPos+1);
        	tokenPos = temp.find("&"); 
        }
        //cout << tmp << endl;
        // Intake initial online user info
        if (tmp == "USER_INITIAL")
       		get_online_users(temp);
        
       	// Intake current friends list
      	if (tmp == "FRIEND_INITIAL"){
       		online.clear();
       		get_friends_list(temp);}

       	// For new registers handle empty friends list
       	if(tmp == "NO_FRIENDS"){
       		cout << "You do not have any friends yet." << endl;
       		online.clear();
       		repeatFlag = 1;
       	}

       	if(tmp == "FRIEND_UPDATE"){
       		//cout << temp << endl;
       		friends.clear();
       		get_friends_list(temp);
       	}

       	// Update online users and if friend display friend online
       	if(tmp == "NEW_LOGIN")
       		new_login(temp);

       	// Handle invites from other users
       	if (tmp == "INVITE"){
       		invite(temp);
       	}

       	if (tmp == "ACCEPTED"){
       		cout << "Friend request accepted:" << endl;
       		cout << temp << endl;
       	}
       	// Server return if attempted to invite non existant user
       	if (tmp == "DNE"){
       		cout << "That username does not exist" << endl;
       	}

       	// Logoff self or other user
       	if(tmp == "LOGOFF"){
       		if (temp == uname){
       			lo_lock.lock();
       			logout = 1;
       			lo_lock.unlock();
       			return;
       		}
       		else
       			logoff_user(temp);
       	}

       	if(tmp == "SHUTDOWN"){
       		if (temp == uname){
       			cout << "\n** Chat server connection error. **" << endl;
       			lo_lock.lock();
       			logout = 1;
       			lo_lock.unlock();
       			disconnect = 1;
       			return;
       		}
       	}

       	    	
   }
} // End update friendds

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////// Register or Login //////////////////////////////////////////////////////

void login(){
	pthread_t connections;
	pthread_t mc;
	string input, pword, combined, payld;
	char *logIn = new char[5];
	size_t i;
	
	cout << "!!!! Welcome to Messenger Chat App !!!!" << endl << endl;

	for(;;){
		repeatFlag = 0;
		logout = 0;
		for(;;){

			// Initial menu for register, login, or exit
			if (disconnect ==0) cout << "What would you like to do?" << endl << endl << "\"r\" to register" << endl <<  "\"l\" to login" << endl << "\"exit\" to exit" << endl; 
			else if (disconnect == 1){
				close(sd); // close server socket on disconnect
				cout << "\n ** You are disconnected from the server, client exiting. **" << endl;
				exit(0);
			}
			cout << "> "; cin >> input;
			
			
			if (input.compare("r") != 0 && input.compare("l") !=0 && input.compare("exit") != 0)
				cout << "Invalid selection.  Try again." << endl;
			else break;
		}
		
		// Loop for user input in case of error.  Good login break from loop
		for (;;){	
			
			// Register new account with server
			if (input.compare("r")==0){
				cout << "Register new account." << endl;
				cout << "Enter a username: "; cin >> uname;
				cout << "Enter a password: "; cin >> pword;
			}
			
			// login with current account
			else if (input.compare("l")==0){
				cout << "Login" << endl;
				cout << "Enter your username: "; cin >> uname;
				cout << "Enter your password: "; cin >> pword;
			}

			else if (input.compare("exit")==0){
				cout << "\nGoodbye." << endl;
				close(sd);
				exit(EXIT_SUCCESS);
			}


		 	
		 	// build string to send
			payld = payload(input, pword);
			send(sd, payld.c_str(), strlen(payld.c_str())+1,0);

			// Read login return status
			if (read(sd, logIn, 5)==0){
				cout << "connection failed.  Exiting" << endl;
				exit(0);
				 
			}
			
			// Handle success or failure
			else{
				//cout << logIn << endl;
				string status(logIn);
				if (status == "200" && input == "r"){
					cout << "\n  **  Congratulations you have registered an account and are logged in!  **\n" << endl;
					break;
				}
				else if (status == "500" && input == "r")
					cout << "That username is taken.  Please select another one." << endl;
				else if (status == "200" && input == "l"){
					cout << "\n  **  You are logged in!  **\n" << endl;
					logged_in = 1;
					break;
				}
				else if (status == "500" && input == "l")
					cout << "Incorrect username or password.  Please try again." << endl;
				
			}
		

			
		}

		// create threads for user input and incoming connections
		//thread mc(main_chat);
		//thread connections(chat_listen); 
	if ((i=pthread_create(&mc, NULL, main_chat, (void*)NULL)) != 0) {
       cout << "failed.";
     }

     if ((i=pthread_create(&connections, NULL, chat_listen, (void*)NULL)) != 0) {
       cout << "failed";
     }
		// maintain read loop for incoming server messages
	update_users();
		
		// Handle logout / disconnects
		if (logout == 1){
			
			// close all sockets except server socket
			sock_lock.lock();
       		for (size_t it = 0; it < all_sockets.size(); it++){
       			if (sd != all_sockets[i])
       				if(close(all_sockets[i])<0)
       					perror("close");			
       		}
       		sock_lock.unlock();
			
			logged_in = 0;
			
			// Cancel and join all threads
			pthread_cancel(connections);
			pthread_join(connections, NULL);
			
			for(i=0;i<threads.size();i++){
				pthread_cancel(threads[i]);
				pthread_join(threads[i],NULL);
			}

			pthread_cancel(mc);
			pthread_join(mc,NULL);

			if (disconnect == 0) cout << "\nGoodbye, " << uname << "!\n" << endl;
			
			// Clear out data structures and cancel and join main chat
			threads.clear();
			friends.clear();
			userList.clear();
			chat_sd.clear();
			repeatFlag = 0;
			pthread_cancel(mc);
			pthread_join(mc,NULL);
				
			
			
			
		}	
	}
}
	
int main(int argc, char** argv){
	string cfile = argv[1];
	logout = 0;
	my_port = "5102";
	struct sigaction sigIntHandler;

	// Signal handling for ctrl-c
    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

	configParse(cfile);
	connect_to_server();
	login();
	return 1;
}