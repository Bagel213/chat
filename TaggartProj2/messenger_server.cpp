
//g++ -Wall -g -std=c++11 -pedantic message_server.cpp
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
#include <map>
#include "server.h"

using namespace std;


// Global Variables
map<string, User*> userList;
int server_port;
string cfile;
string ufile;
bool sflag;

//////////////////////////////////////////////////// Functions for User Class ///////////////////////////////////////////////////

// maintain user's name and password
void User::name_and_password(string n, string p){
    username = n;
    password = p;
}
// add friend to vector of friends
void User::addFriend(string f){
    friends.push_back(f);
}
//set self_online = 1
void User::online(int sd){
    self_online = 1;
    my_sd = sd;
}
// set self_online = 0
void User::offline(){
    self_online = 0;
}

// Keep track of users IP and port
void User::set_IP_port(string i, string p){
    ip = i;
    port = p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////// signal handler and shutdown file save //////////////////////////////////////////////

void sig_handler(int s){
    
    User *user;
    int sd;
    size_t i;
    string toFile;
    ofstream outfile;
    outfile.open(ufile);



    cout << "\nServer shutting down......." << endl;
    if (!userList.empty()){
        for(auto iter = userList.begin(); iter!=userList.end(); ++iter){ // Iterate through all online users and send shutdown
            toFile = "";
            user = iter->second;
            sd = user->my_sd;
            string lo = "SHUTDOWN&" + user->username;
            send(sd, lo.c_str(), strlen(lo.c_str())+1,0);
            

            toFile = user->username + "|" + user->password + "|"; // Iterate through all users and build new user_file_info
            if (user->friends.size() != 0){
	            for (i=0; i<user->friends.size()-1;i++){
	                user->friends[i].erase(user->friends[i].find_last_not_of("\n") + 1);
	                toFile+=user->friends[i];
	                if (toFile[toFile.size()-1]!=';')
	                    toFile += ";";
	            }
	            toFile+=user->friends[i]+"\n";
        	}
            else toFile+="\n";
            outfile << toFile;
        }       
    }
    outfile.close();
    exit(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////// Handle Invites ////////////////////////////////////////////////////////////


// New invite incoming
void invite(string incoming, int inviter_sd){
    string temp(incoming);
    size_t tokenPos;
    vector<string> tmp;
    string inviter, invitee, message, payload;
    User *user;
    int invitee_sd;

   
    // Parse incoming info
    tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
        tmp.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find("|");  
        }
    tmp.push_back(temp.substr(0, tokenPos));
    inviter = tmp[1]; invitee = tmp[4]; message = tmp[5];
    
    // Send invitation and message to invitee
    if (userList.find(invitee) != userList.end()){
        user = userList.at(invitee);
        invitee_sd = user->my_sd;
        payload = "INVITE&" + inviter + "|" + message;
        send(invitee_sd, payload.c_str(), strlen(payload.c_str())+1,0);
    }
    
    //if user does not exit reply DNE
    else{
        payload = "DNE&";
        send(inviter_sd, payload.c_str(), strlen(payload.c_str())+1,0);
    }
}


// Once invite accepted
void accept_invite(string temp){

    size_t tokenPos;
    vector<string> tmp, tmpB;
    string unameA, unameB;
    User *user;
    int sdA, sdB;
    string pyld;
   // string friends = "FRIEND_UPDATE&"; // String header

    // Parse incoming info
    tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
        tmp.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find("|");  
        }
    tmp.push_back(temp.substr(0, tokenPos));
    
    temp = tmp[4];
    tokenPos = temp.find(";"); 
    while (tokenPos != string::npos){
        tmpB.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find(";");  
        }
    tmpB.push_back(temp.substr(0, tokenPos));
    
    
    // Send accepted message to inviter
    unameA = tmp[1]; unameB = tmpB[0];
    //cout << unameA << " " << unameB << endl;
    pyld = "ACCEPTED&"+ unameA + ">>" +tmpB[1];
    user = userList.at(unameB);
    send(user->my_sd, pyld.c_str(),strlen(pyld.c_str())+1,0);
   

    //Update friends list for both parties
    user = userList.at(unameA);
    user->friends.push_back(unameB);
    sdA = user->my_sd;
    send_users(sdA, unameA, 1);


    user = userList.at(unameB);
    user->friends.push_back(unameA);
    sdB = user->my_sd;
    send_users(sdB, unameB, 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////// Register and Login function ///////////////////////////////////////////////

//Disconnect logoff
void disconnect_logoff(string uname){
       
    User *user;
   
    user = userList.at(uname);
    user->self_online = 0;
    update_all(uname, 0); // send online update to all
}


// Get logoff username
void logoff(string incoming, int sd){
    
    //cout << "logoff function" << endl;
    string temp(incoming);
    size_t tokenPos;
    vector<string> tmp;
    User *user;
   
    // Parse incoming info
    tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
        tmp.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find("|");  
        }
    tmp.push_back(temp.substr(0, tokenPos));
    user = userList.at(tmp[1]);
    user->self_online = 0;
    update_all(tmp[1], 0); // send online update to all
    string lo = "LOGOFF&" + tmp[1]; // reply to client wanting to log out
    send(sd, lo.c_str(), strlen(lo.c_str())+1,0);

}

// Register new user
string new_account(char *incoming, int sd, struct sockaddr_in srvaddr){

    string temp(incoming);
    size_t tokenPos;
    vector<string> tmp;
    User *user = new(User);
    string good = "200";
    string bad = "500";
    string uname = "none";
    string pword, ip;
    //int addrlen = sizeof(srvaddr);

    // Parse incoming info
    tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
        tmp.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find("|");  
        }
    tmp.push_back(temp.substr(0, tokenPos));
    uname = tmp[1]; pword = tmp[4]; // Username and password
    //cout << tmp[1] << " " << tmp [4] << " " << tmp[2] << " " << tmp[3] << endl;
    if (userList.find(uname) == userList.end()){
        send(sd, good.c_str(), strlen(good.c_str())+1,0); // Send confirmation that name is available
        
        // Create new user and log them in
        user->name_and_password(tmp[1],tmp[4]);
        user->online(sd);
        //getpeername(sd, (struct sockaddr*)&srvaddr, (socklen_t*)&addrlen);   // get new client ip address
        ip = tmp[2]; //inet_ntoa(srvaddr.sin_addr);
        user->set_IP_port(ip,tmp[3]);
        user->new_account = 1;
        userList.insert(pair<string,User*>(user->username,user)); // Add to map
    }
    else{
        send(sd, bad.c_str(), strlen(bad.c_str())+1,0); // Username exist
        uname = "none";
    }
    return uname;
} // end new_account


string login(char *incoming, int sd, struct sockaddr_in srvaddr){

    string temp(incoming);
    size_t tokenPos;
    vector<string> tmp;
    string uname = "none";
    string pword;
    User *user;
    string good = "200";
    string bad = "500";
    string ip;
    //int addrlen = sizeof(srvaddr);
    

    // Parse incoming info
    tokenPos = temp.find("|"); 
    while (tokenPos != string::npos){
        tmp.push_back(temp.substr(0, tokenPos));
        temp.erase(0, tokenPos+1);
        tokenPos = temp.find("|");  
        }
    tmp.push_back(temp.substr(0, tokenPos));
    uname = tmp[1]; pword = tmp[4]; // Username and password
   

    // Check if username exist in the map userList and if so sign in
    if (userList.find(uname) == userList.end()){
        send(sd, bad.c_str(), strlen(bad.c_str())+1,0); // Bad username
        uname = "none";
    }
    else {
        user = userList.at(uname);
        if (pword == user->password){
            user->online(sd); // Set status of user object in map to online for login user
            //getpeername(sd, (struct sockaddr*)&srvaddr, (socklen_t*)&addrlen);  // get new login ip address
            ip = tmp[2]; //inet_ntoa(srvaddr.sin_addr);
            user->set_IP_port(ip,tmp[3]); // Maintain port and ip
            user -> new_account = 0;
            cout << uname << " at IP " << ip << " logged in." << endl; // Print to server terminal
            send(sd, good.c_str(), strlen(good.c_str())+1,0); // Login success
        }
        else{
         send(sd, bad.c_str(), strlen(bad.c_str())+1,0); // Bad password
         uname = "none";
        }
    }

  
    return uname;
        

} // end login


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////// Send online users and friends list ////////////////////////////////////////////////


//Update Friends
void update_all(string uname, int flag){
    
    string online;
    User *user = userList.at(uname);
    string ip = user->ip;
    string port = user->port;
    int usd;

    // send update of online users to all
    for(auto i = userList.begin(); i!=userList.end(); ++i){
        user = i->second;
        
        if (user->self_online == 1 && user->username != uname){
            //cout << "online user " << i->first << endl;
            if (flag == 1) online = "NEW_LOGIN&";
            if (flag == 0) online = "LOGOFF&";
            usd = user->my_sd;
            online += uname + ";" + ip + ";" + port;
            send(usd, online.c_str(), strlen(online.c_str())+1, 0);
        }

    }
}


// Send Friend and global user info to client
void send_users(int sd, string curr_uname, int flag){
    //cout << "send user "<< curr_uname << endl;
    map<string, User*>::iterator iter;
    vector<string>::iterator iterV;
    vector<string> temp;
    string friends;
    if (flag == 0) friends = "FRIEND_INITIAL&"; // String header
    else if (flag == 1) friends = "FRIEND_UPDATE&"; // String header
    string uname, tempFriend;
    User *user;
    User *curr = userList.at(curr_uname);
    string online = "USER_INITIAL&"; // String header
    size_t i;

    
    
        // Send all online users
        for (iter = userList.begin(); iter != userList.end(); iter++){ // Iterate through all users and check for online
            uname = iter->first;
            user = userList.at(uname);
            if ((user->self_online == 1) && (uname != curr_uname)){  // Build string to send
                if(find(curr->friends.begin(), curr->friends.end(), uname) != curr->friends.end())
                //cout << uname << endl;
                    online += uname + ";" + user->ip + ";" + user->port + "|";
            }
        }
        //cout << online << endl;
            send(sd, online.c_str(), strlen(online.c_str())+1,0); // Send all users online
    

     // send friends list
    user = userList.at(curr_uname);
    if(user->friends.size() !=0){
        temp = user->friends;
        for (i=0; i < temp.size(); i++){ // Build string to send
           friends.append(temp[i]);
           friends.append("|");
        }
       //friends.append(temp[i]);
        sleep(1);
        send(sd, friends.c_str(), strlen(friends.c_str())+1,0); // Send Friends list
    }
    else{
        friends = "NO_FRIENDS&";
        sleep(1);
        send(sd, friends.c_str(), strlen(friends.c_str())+1,0); // Send Friends list
    }
    
   

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////// Read and Parse input files ////////////////////////////////////////////////


// Open and read user_info_file and configuration_file 
// Then parse for usage
void user_config(){
    string line, config, token;
    ifstream user_file(ufile);
    ifstream config_file(cfile);
    vector<string>::iterator iter;
    vector<string> user_info, tempConfig;
    sflag = 0;
    

    // Open and read user info
    if (user_file.is_open()){
        while(getline(user_file, line)){
            user_info.push_back(line);
        }
        user_file.close();
    } 
    else cout << "user info file failed to open";

     // Open and read configuration
    if (config_file.is_open()){
        while (config_file >> line)
            tempConfig.push_back(line);
        config_file.close();
    } 
    else cout << "configuration_file failed to open";
    server_port = stoi(tempConfig[1]);
    if (server_port == 0)
    	sflag =1;
    
    // Parse the user info
    size_t tokenPos;
    vector<string> tmp;
    string temp; 
    int i = 0;
    User *user;

    for (iter = user_info.begin(); iter != user_info.end(); ++iter){
        
        user = new(User); // Create new user object
        temp = user_info[i];
        i++;

        // Parse username and password
        tokenPos = temp.find("|"); 
        while (tokenPos != string::npos){
            tmp.push_back(temp.substr(0, tokenPos));
            temp.erase(0, tokenPos+1);
            tokenPos = temp.find("|");  
        }
        user->name_and_password(tmp[0], tmp[1]); // Set name and password

        // Parse Friends list
        tmp.clear();
        tokenPos = temp.find(";");
        while (tokenPos != string::npos){
            user->addFriend(temp.substr(0, tokenPos));
            temp.erase(0, tokenPos+1);
            tokenPos = temp.find(";");  
        }
        user->addFriend(temp); // Add to user objects friend list
        userList.insert(pair<string,User*>(user->username,user)); // Add user to map of user objects
        user = NULL;
    }


} // end user_config


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////// Bind Sockets ////////////////////////////////////////////////////////


// Bind socket and listen
void bindSocket(){
	
	int cfd; //socket descriptor
	int opt=1; // used in setting options for socket
	struct sockaddr_in srvaddr; // struct for server IP and port numbers


	 // Create file descriptor for the socket
	if ((cfd=socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed.  Ending C&C\n");
		exit(EXIT_FAILURE);
	}


	// Set up srvaddr struct
	bzero(&srvaddr, sizeof(srvaddr)); // Zero out all fields
	srvaddr.sin_family = AF_INET;     // Set to IPv4
	srvaddr.sin_port = htons(server_port);    // Set port to 5551, htons converts from host to network byte order
	srvaddr.sin_addr.s_addr = INADDR_ANY; // Allow any address to connect


	// Set socket level options to allow local reuse of addr and port
	if (setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt failed.  Ending\n");
		exit(EXIT_FAILURE);
	}


	// Bind the socket to our desired port
	if (bind(cfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr))<0){
		perror("bind failed.  Ending\n");
		exit(EXIT_FAILURE);
	}

	
	// Set socket to accept incoming connections with queue size of 5
	if(listen(cfd, 5)<0){
		perror("listen failed.  Ending\n");
		exit(EXIT_FAILURE);
	}

 	if (sflag == 1){
		int len = sizeof(srvaddr);
        getsockname(cfd, (struct sockaddr *)&srvaddr, (socklen_t *)&len);
        cout << "\n** Configuration port was 0.  System chosen port = " <<  ntohs(srvaddr.sin_port) << " **\n" << endl;
	}

	
	// New thread for handling incoming connections
	connectClients(srvaddr,cfd);



	return;

} // end bindSocket()


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////// Connect Clients ////////////////////////////////////////////////////////


// Connect Clients 
void connectClients(struct sockaddr_in srvaddr, int cfd){
	
	int temp_socket, sd, max_sd, r, count = 0;
	vector<int> client_sockets(1024,0);
	vector<int>::iterator iter;
    char *incoming = new char[1024];
    string connected = "connected";
    string uname, pword, temp, tmp;
    User *user;

	//set of socket descriptors  
    fd_set clientfds;

	int addrlen = sizeof(srvaddr);
	
	// Loop for client connection up to max clients
	for(;;){
		
		//clear the socket set  
        FD_ZERO(&clientfds);   
     

        //add master socket to set  
        FD_SET(cfd, &clientfds);   
        max_sd = cfd;  
	

		//add child sockets to set  
    	for (size_t i=0;i<client_sockets.size();i++){   
        
        	// set socket descriptor and add to vector
        	sd = client_sockets[i];     
            FD_SET(sd, &clientfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        } 


        // wait for activity on a socket with timeout = NULL
        if((select( max_sd + 1 , &clientfds , NULL , NULL , NULL)<0) && errno!=EINTR)
        	cout << "select error";


        // Something happens on the primary socket = new connection
        if (FD_ISSET(cfd, &clientfds)){   
            if ((temp_socket = accept(cfd, (struct sockaddr *)&srvaddr, (socklen_t*)&addrlen))<0){   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }  
            else{
            	// print connect details and add to vector
            	printf("Client connected , ip %s\n", inet_ntoa(srvaddr.sin_addr)); 
                for (size_t i = 0; i!=client_sockets.size();i++){
                    if (client_sockets[i] == 0){
                        client_sockets[i] = temp_socket;
                        break;
                    }
                }
                //client_sockets.push_back(temp_socket);   // Add to vector of sockets
                send(temp_socket, connected.c_str(), strlen(connected.c_str())+1, 0); 

            }
        }


       // Communicate with connected clients
       for (size_t i = 0; i!=client_sockets.size();i++){
        	if (client_sockets[i]!=0){
        		sd = client_sockets[i];   
        	
        		// if activity detected on sd.  Should only be when the client disconnects.  All other handled below.
        		if (FD_ISSET(sd, &clientfds)){
        			
        			// Wait to read if client connected on sd. If disconnected read will return 0.
        			if ((r = read(sd, incoming, 1024)) == 0){   
                    	
                    	//Somebody disconnected , get details and print  
                    	getpeername(sd, (struct sockaddr*)&srvaddr, (socklen_t*)&addrlen);   
                    	printf("Client disconnected , ip %s\n", inet_ntoa(srvaddr.sin_addr));   
                         
                    	//Close the socket and remove from vector
                    	close(sd);
                        for (auto i=userList.begin(); i!=userList.end(); i++){
                            user = userList.at(i->first);
                            if (sd == user->my_sd && user->self_online == 1){
                                disconnect_logoff(user->username);
                                count--;
                                cout << count << " users logged in." << endl;
                            }                         
                        }
                        client_sockets[i] = 0;
                    	//if (client_sockets.empty()) break; // break out of for loop since vector could be empty
                	}

                    
                    // Handle i/o with connected client
                    else{ 
                        temp = incoming;

                        // Create new account
                        if (incoming[0] == 'r'){
                            uname = new_account(incoming,sd, srvaddr); 
                            //cout << uname << endl;
                            if (uname != "none"){
                                send_users(sd, uname, 0); // send login client online users and friend list
                                count++;
                                cout << count << " users logged in." << endl;
                            }
                            //update_all(uname, 1);
                           
                        }
                        
                        // User logoff
                        else if(temp.substr(0,6) == "logoff"){
                            //cout << "logoff" << endl;
                            logoff(incoming, sd);
                            count--;
                            cout << count << " users logged in." << endl;

                        }
                        // Login
                        else if (incoming[0] == 'l'){
                            if ((uname = login(incoming,sd, srvaddr)) != "none"){ // Login function
                                send_users(sd, uname, 0); // send login client online users and friend list
                                update_all(uname, 1);
                                count++;
                                cout << count << " users logged in." << endl;
                                
                            }
                        }

                        // user accepts invite
                         else if(temp.substr(0,2) == "ia")
                            accept_invite(temp);


                        // Handle invites
                        else if (incoming[0] == 'i'){
                            invite(incoming, sd);
                        } 

                        else if (incoming[0] == 'd'){
                            cout << "Client Ctrl-C initiated." << endl;
                        }



                    }
            	} 
            }    	
        }
    }

    return;
} // end connectClient()


int main(int argc, char** argv){
	ufile = argv[1];
    cfile = argv[2];
    
    cout << "\nHello Dave, the chat server is online.\n" << endl;
    cout << "Ctrl-C to exit"<< endl;
    struct sigaction sigIntHandler;

    // handle ctrl-c
    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    //primary functions
    user_config();
    bindSocket();
	return 1;
}
