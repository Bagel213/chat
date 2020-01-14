#ifndef _SERVER_H
#define _SERVER_H

// Function declarations
void connectClients(struct sockaddr_in srvaddr, int); 		 // Listen and accept incoming connections
void user_config();    										 // Get config file
std::string new_account(char *, int, struct sockaddr_in);	 // Register new user
std::string login(char *, int, struct sockaddr_in); 		 // Login user
void send_users(int, std::string, int);  				     // send online users and friend list to new login
void update_all(std::string, int); 							 // Update all as to what friends are online
void logoff(std::string, int); 								 // update user info for user logging off
void invite(std::string,int); 								 // Handle invites
void accept_invite(std::string); 							 // Handle invite accepts by updating friends lists
void sig_handler(int); 										 // Handle ctrl-c
void disconnect_logoff(std::string); 						 // handle logging off for a disconnection

// Classes
class User {
    
    public:
        // Class variables
        std::string username, password;
        std::vector<std::string> friends;
        bool self_online;
        std::string ip;
        std::string port;
        int my_sd;
        bool new_account;
        
        // Class Constructor
        User(){
            self_online = 0;
        }
        
        // Class destructor
        ~User(){}
    
        // Class functions
        void name_and_password(std::string, std::string);  // maintain user's name and password
        void addFriend(std::string);                       // add friend to vector of friends
        void online(int);                                  // set self_online = 1
        void offline();                                    // set self_online = 0
        void set_IP_port(std::string, std::string);        // Keep track of user's ip and port

       
};

#endif