#ifndef _CLIENT_H
#define _CLIENT_H

// classes
class User {
	
	public:
		
		std::string username;
		std::string ip;
		std::string port;
		bool is_friend = 0;

		// Class Constructor
        User(){
            is_friend = 0;
        }
        
        // Class destructor
        ~User(){}

        // Class Funcitions
		void logged_off(); // Notify of logged off friends
		void my_friend();  // set user to a friend
};

// Function declarations
void configParse(std::string); 					// Parse the configuration file
void connect_to_server(); 						// connect to server and recieve reply
void login(); 									// Register, login, or exit.
void update_users();							// Get list of online users and add to online friend lists
std::string payload(std::string, std::string);	// Add standard "header" to packets
void print_online_friends();					// Print online friends
void get_online_users(std::string);				// Get all online friends
void get_friends_list(std::string);				// Get friends list
void new_login(std::string);					// If new login to server update online list
void logoff_user(std::string);					// Handle logoff
void chat_listen();								// Bind to port to listen for incoming chat
std::string build(std::string);					// build header
void invite(std::string);						// Handle sending invite
void *friend_chat_outgoing(void*);				// thread for outgoing chat
void *friend_chat_incoming(void*);				// thread for incoming chat
void sig_handler(int);							// handle ctrl-c
void *connections(void *);						// thread for listening for connections
void *main_chat(void*);							// main chat thread
#endif