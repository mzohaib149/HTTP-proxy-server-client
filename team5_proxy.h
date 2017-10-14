#ifndef SERVER_H_
#define SERVER_H_
#include "team5_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <iostream>
#include <deque>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <map>
using namespace std;

struct cache_entry
{
	time_t date_accessed;
	time_t last_modified;
	time_t expires;
	bool expires_exists;
	std::string request;
	std::string response;
	std::map<std::string,std::string> response_map;//contains response keys and values pairs
};

struct socket_pair
{
	std::string request;
	int client_to_proxy_socket;
	int proxy_to_server_socket;
};

void print_cache();
void delete_cache_entry(int i);
void modify_request_message(std::string& buffer_string, std::string& date_modified);
void time_string_to_time_stamp(time_t &timestamp, const char* timestring);
void time_stamp_to_time_string(time_t timestamp, char* timestring);
void handle_new_connection(int listenfd, sockaddr_in client_addr, fd_set &allset, int &maxfd);
void got_data(int descriptor, fd_set &allset, int &maxfd);
#endif
