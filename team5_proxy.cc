#include "team5_common.h"
#include "team5_proxy.h"

deque<cache_entry> cache;
deque<socket_pair> socket_pair_list;

void print_cache()
{
	cout<<"printing cache..."<<endl;
	cout<<"*******************************"<<endl;		
	for (int i=0; i<cache.size(); i++)
	{
		cout<<"entry no.: "<<(i+1)<<endl;
		cout<<"request message: "<<endl;
		cout<<cache[i].request<<endl;
		cout<<"response message: "<<endl;
		//cout<<cache[i].response<<endl;
		cout<<"last modified: "<<endl;
		char last_modified[100];
		memset(last_modified,0,100);
		time_stamp_to_time_string(cache[i].last_modified, last_modified);
		cout<<last_modified<<endl;
		cout<<"timestamp: "<<endl;
		cout<<cache[i].last_modified<<endl;
		cout<<"date accessed: "<<endl;
		
		char date_accessed[100];
		memset(date_accessed,0,100);
		time_stamp_to_time_string(cache[i].date_accessed, date_accessed);
		cout<<date_accessed<<endl;
		cout<<"timestamp: "<<endl;
		cout<<cache[i].date_accessed<<endl;
		if (cache[i].expires_exists)
		{
			cout<<"expires header exists"<<endl;
			cout<<"expires: "<<endl;
			char expires[100];
			memset(expires,0,100);
			time_stamp_to_time_string(cache[i].expires, expires);
			cout<<expires<<endl;
			cout<<"timestamp: "<<endl;
			cout<<cache[i].expires<<endl;
		
		}
		else 
		{	
			cout<<"expires header does not exist"<<endl;
		}
	}
	cout<<"*******************************"<<endl;
}

void delete_cache_entry(int i)
{
	cache.erase(cache.begin()+i);
}


void modify_request_message(std::string& buffer_string, std::string& date_modified)
{
	std::string str_buffer;
	str_buffer = buffer_string;
	std::remove(str_buffer.begin(),str_buffer.end(),'\r');
	std::string str;
	std::stringstream stream(str_buffer);
	getline(stream,str);
	std::string output_string;
	output_string=str;
	while(std::getline(stream, str))
	{
		//cout<<str;
		std::string temp;
		temp = "\r\n"+str;
		
			
		output_string=output_string+temp;
		temp.clear();
		if (str.length()==0)
			break;
		str.clear();
		
	}
	str.clear();
	output_string=output_string+"If-modified-since: "+ date_modified+"\r\n"+"\r\n";
	
	buffer_string.clear();
	buffer_string = output_string;
	
}

void time_string_to_time_stamp(time_t &timestamp, const char* timestring)//converts time given in string format to time_t variable
{
	struct tm timeinfo;
	strptime(timestring, "%a, %e %b %Y %T GMT", &timeinfo);
	timestamp = timegm(&timeinfo);
}

void time_stamp_to_time_string(time_t timestamp, char* timestring)//converts time given in time_t variable to string format
{
	struct tm* timeinfo;	
	timeinfo = gmtime(&timestamp);
	strftime(timestring, 100, "%a, %e %b %Y %T GMT",timeinfo);
}

void handle_new_connection(int listenfd, sockaddr_in client_addr, fd_set &allset, int &maxfd)
{
	cout<<"got a new connection"<<endl;
	socklen_t addr_len = sizeof(client_addr);
	int newsock;
	if ((newsock = accept(listenfd, (struct sockaddr*)& client_addr, &addr_len))<0)
		perror("accept error"); 

	FD_SET(newsock, &allset);
	
	
	if (newsock>maxfd)
	{
		maxfd = newsock;
	}
}

void got_data(int descriptor, fd_set &allset, int &maxfd)
{
	char buffer[BUFF_SIZE];
	memset(buffer,0,BUFF_SIZE);
	
	int nbytes = recv(descriptor, buffer, BUFF_SIZE, 0);
	
	 
	
				
	cout<<"received "<<nbytes<<" bytes of data"<<endl;
 	
	if (nbytes == 0)// connection is closed by some client
	{
		close(descriptor);
		FD_CLR(descriptor,&allset);
	}
	else
	{
		int buffer_size = nbytes;
		std::string str_buffer(buffer);
		if (str_buffer.substr(0,3) == "GET")//we received request message from client
		{
			//extract server name from request
			cout<<"Request message received from client"<<endl;
			std::string tempbuffer=str_buffer;
			std::remove(tempbuffer.begin(),tempbuffer.end(),'\r');
			std::string str;
			std::stringstream stream(tempbuffer);
			getline(stream,str);
			str.clear();
			getline(stream,str);
			std::string::size_type index;
			index = str.find(':', 0);
			std::string hostname=str.substr(index+2);
			cout<<hostname<<endl;
			const char* servername=hostname.c_str();
			//get IP from server name
			char server_ip_string[50];		
			struct addrinfo hints, *res, *p;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			getaddrinfo(servername, NULL, &hints, &res);
			
			for (p=res;p!=NULL;p=p->ai_next)
			{
				void* addr;
				if (p->ai_family == AF_INET)
				{
					struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
					addr = &(ipv4->sin_addr);
				}
				inet_ntop(p->ai_family, addr, server_ip_string, sizeof(server_ip_string));
				cout<<"IP address of server is "<<server_ip_string<<endl;
			}
			//create new socket
			int sockfd;
			if ((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
			{
				perror("socket error");
				exit(0);
			}
			struct sockaddr_in server_addr;
			bzero(&server_addr,sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(80);
			inet_pton(AF_INET,server_ip_string,&(server_addr.sin_addr));
			if ((connect(sockfd,(struct sockaddr*) &server_addr, sizeof(server_addr)))<0)
			{
				perror("connect error");
				exit(0);
			}
			FD_SET(sockfd,&allset);
			if (sockfd>maxfd)
			{
				maxfd = sockfd;
			}
			
			//create client and server socket mapping
			struct socket_pair sp;
			sp.client_to_proxy_socket=descriptor;
			sp.proxy_to_server_socket=sockfd;
			sp.request = str_buffer;
			socket_pair_list.push_back(sp);
			if (cache.size()==0)//if cache is empty
			{
				cout<<"Cache is empty"<<endl;
				cout<<"Compulsory miss!"<<endl;
				std:string buffer_string(buffer);
				struct cache_entry ce;
				ce.request = buffer_string;
				cache.push_front(ce);
				int bytes_sent = send(sockfd, buffer, buffer_size, 0);
				cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
			}
			else
			{	
				//find cache entry
				if (cache.size()==MAX_CACHE_SIZE)
					cache.pop_back();

				bool flag=false;
				int position=0;
				for(int i=0; i<cache.size();i++)
				{
					//if (!strcmp(cache[i].request.c_str(),buffer))
					if(cache[i].request == str_buffer)
					{
						flag=true;
						position=i;
						break;
					}
				}
				if(!flag)//entry doesn't exist in cache
				{
					cout<<"Cache Miss!"<<endl;
					std::string buffer_string(buffer);
					struct cache_entry ce;
					ce.request = buffer_string;
					cache.push_front(ce);
					int bytes_sent = send(sockfd, buffer, buffer_size, 0);
					cout<<"sent the request to server"<<endl;
					cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
				}
				else//entry exists in cache
				{
					cout<<"Cache Hit!"<<endl;
					
					//**********************************
					//if expires header exists
					//check if entry is not expired
					//if it is not expired
					//check to see if its fresh 
					//if its fresh 
					//
					//send if-modified-since message
					//else
					//remove it
					//create new entry
					//**********************************
					struct timeval now;
					gettimeofday(&now,NULL);
					
					if (cache[position].expires_exists)
					{
						cout<<"expires header exists"<<endl;
						if (cache[position].expires > now.tv_sec)//entry is not expired
						{
							cout<<"expires in header is greater than current time"<<endl;
							if ((now.tv_sec - cache[position].date_accessed <= 24*60*60) && (now.tv_sec - cache[position].last_modified <= 30*24*60*60))//entry is fresh
							{
							//in the map associated with cache entry, find the value with key Last_Modified					
								cout<<"entry was accessed in last 24 hours and it was last modified within a month"<<endl; 
								std::map<std::string,std::string>::iterator it= cache[position].response_map.find("Last-Modified");

								std::string date_modified = it->second;
								//edit the request message by adding "If-modified-since"
								std::string buffer_string(buffer);
								modify_request_message(buffer_string,date_modified);
								const char* buffer_to_send = buffer_string.c_str();
								//create temp entry ... delete original and put it at front of cache
								struct cache_entry temp_ce;
								temp_ce.date_accessed=cache[position].date_accessed;
								temp_ce.last_modified=cache[position].last_modified;
								temp_ce.request=cache[position].request;
								temp_ce.response=cache[position].response;
								temp_ce.response_map=cache[position].response_map;
								delete_cache_entry(position);
								cache.push_front(temp_ce);	
								int bytes_sent = send(sockfd, buffer_to_send, 1024, 0);
								cout<<"sent conditional GET to server"<<endl;
								cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
							}

							else//not fresh according to expires header
							{
								cout<<"Entry was accessed beyond last 24 hours or was modified a month ago or earlier"<<endl;
								cout<<"Entry is stale and is deleted"<<endl;
								delete_cache_entry(position);
								std::string buffer_string(buffer);
								struct cache_entry ce;
								ce.request = buffer_string;
								cache.push_front(ce);
								int bytes_sent = send(sockfd, buffer, buffer_size, 0);
								cout<<"sent new GET message to server"<<endl;
								cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
							}
						}
						else //entry is expired
						{
							cout<<"expires in header is less than current time"<<endl;
							cout<<"Entry is stale and is deleted"<<endl;
							delete_cache_entry(position);
							std::string buffer_string(buffer);
							struct cache_entry ce;
							ce.request = buffer_string;
							cache.push_front(ce);
							int bytes_sent = send(sockfd, buffer, buffer_size, 0);
							cout<<"sent new GET message to server"<<endl;
							cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;

						}
					}
					else//expires header doesn't exist
					{
						cout<<"expires header doesn't exist"<<endl;
						if ((now.tv_sec - cache[position].date_accessed <= 24*60*60) && (now.tv_sec - cache[position].last_modified <= 30*24*60*60))//entry is fresh
						{
							//in the map associated with cache entry, find the value with key Last_Modified					
								cout<<"entry was accessed in last 24 hours and it was last modified within a month"<<endl;
							
								std::map<std::string,std::string>::iterator it= cache[position].response_map.find("Last-Modified");

								std::string date_modified = it->second;
								//edit the request message by adding "If-modified-since"
								std::string buffer_string(buffer);
								modify_request_message(buffer_string,date_modified);
								const char* buffer_to_send = buffer_string.c_str();
								//create temp entry ... delete original and put it at front of cache
								struct cache_entry temp_ce;
								temp_ce.date_accessed=cache[position].date_accessed;
								temp_ce.last_modified=cache[position].last_modified;
								temp_ce.request=cache[position].request;
								temp_ce.response=cache[position].response;
								temp_ce.response_map=cache[position].response_map;
								delete_cache_entry(position);
								cache.push_front(temp_ce);	
								int bytes_sent = send(sockfd, buffer_to_send, 1024, 0);
								cout<<"sent conditional GET to server"<<endl;
								cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
							}

							else//not fresh
							{
								cout<<"Entry was accessed beyond last 24 hours or was modified a month ago or earlier"<<endl;
								cout<<"Entry is stale and is deleted"<<endl;
								delete_cache_entry(position);
								std::string buffer_string(buffer);
								struct cache_entry ce;
								ce.request = buffer_string;
								cache.push_front(ce);
								int bytes_sent = send(sockfd, buffer, buffer_size, 0);
								cout<<"sent new GET message to server"<<endl;
								cout<<"sent "<<bytes_sent<<" bytes of request to server"<<endl;
							}
					}
					
				}
			}	
		}
		else if (str_buffer.substr(0,5) == "HTTP/")  //we received the first response message from server
		{
			//find the client socket address and webpage
			cout<<"response received from server"<<endl;
			std::string temp_request;
			temp_request.clear();
			int temp_client_socket;
			int position=0;
			for (int x=0; x<socket_pair_list.size(); x++)
			{
				if (socket_pair_list[x].proxy_to_server_socket == descriptor)
				{
					temp_client_socket = socket_pair_list[x].client_to_proxy_socket;
					temp_request = socket_pair_list[x].request;
					position = x; 
					break;
				}
			}
			socket_pair_list.erase(socket_pair_list.begin()+position);
			//find entry with temp_request in cache
			int y=0;
			for (y=0; y<cache.size(); y++)
			{
				if (cache[y].request == temp_request)
				{
					break;
				}
			}
			
			if (str_buffer.substr(9,3) == "304")//we received response with not modified page
			{	
				
					cout<<"we received response with not modified page"<<endl;
					//send corresponding response on temp_client_socket
					const char* buf = cache[y].response.c_str();
					print_cache();
					int bytes_sent = send(temp_client_socket, buf, cache[y].response.size(), 0);
					cout<<"sent entry from cache"<<endl;
					cout<<"sent "<<bytes_sent<<" bytes of response to client"<<endl;
				
			}
			
			else			
			{
					//update the entry in cache 
					//don't forget to put date_accessed && last_modified timestamps in cache
					//send the response on temp_client_socket
				cout<<"we received response with new or modified page"<<endl;
				cache[y].response = str_buffer;
				//populate the map
				std::string temp_string(buffer);
				cout<<buffer<<endl;
				std::remove(temp_string.begin(),temp_string.end(),'\r');
				std::string str;
				
				std::stringstream stream(temp_string);
				getline(stream,str);
				str.clear();				
				std::string::size_type index;
				cache[y].expires=0;//if expires exists it will be overridden
				cache[y].expires_exists = false;//if expires exists it will be overridden
				bool f=false;
				cache[y].last_modified=0;
				while(1)
				{
					getline(stream, str);
					if (str.length()==0)
						break;
					index =0;
					index = str.find(':', 0);
					//cout<<index<<endl;
					cache[y].response_map.insert(std::pair<std::string,std::string>(str.substr(0,index),str.substr(index+2)));
					if ((str.substr(0,index))=="Date")
					{
						time_string_to_time_stamp(cache[y].date_accessed, str.substr(index+2).c_str());
						str.clear();
						
					}
					else if ((str.substr(0,index))=="Last-Modified")
					{
						time_string_to_time_stamp(cache[y].last_modified, str.substr(index+2).c_str());
						str.clear();
						
					}
					else if (((str.substr(0,index))=="Expires") && (f==false))
					{
						time_string_to_time_stamp(cache[y].expires, str.substr(index+2).c_str());
						
						cache[y].expires_exists = true;
						f=true;
						index=0;
						str.clear();
						
					}
					
					
				}
				if (cache[y].last_modified==0)
					cache[y].last_modified = cache[y].date_accessed;
				
				///////////////////////////////
				while(1)
				{
					char another_buffer[BUFF_SIZE];					
					memset(another_buffer,0, BUFF_SIZE);
					int numbytes = recv(descriptor, another_buffer, BUFF_SIZE, 0);
					
					if (numbytes>0)
					{
						cout<<"received another "<<numbytes<<" from server"<<endl;
						std::string temp(buffer);
						cache[y].response = cache[y].response + temp;
					} 	
					else if (numbytes == 0)
					{	
						print_cache();
						int bytes_sent = send(temp_client_socket, cache[y].response.c_str(), cache[y].response.size(), 0);
						//cout<<"sent entry from cache"<<endl;
						cout<<"sent "<<bytes_sent<<" bytes of response to client"<<endl;
						break;
					}
				}
				close(descriptor);
				FD_CLR(descriptor,&allset);
				
			}
			
		}
		
	}
	
}



int main(int argc,char** argv)
{
	//unsigned short int client_count = 0;
	if (argc!=3)
	{
		cout<<"Please enter 2 arguments"<<endl;
		cout<<"quitting ..."<<endl;		
		return 0;
	}
	char proxy_server_ip[50];
	char proxy_server_port_string[50];
	unsigned short int port;	
	strcpy(proxy_server_ip,argv[1]);
	strcpy(proxy_server_port_string,argv[2]);
	port = atoi(proxy_server_port_string);
	cout<<proxy_server_ip<<" "<<port<<endl;
	int listenfd;
	struct sockaddr_in proxyservaddr,client_addr;	
	
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("socket error");
		return 0;
	}
	
	bzero(&proxyservaddr, sizeof(proxyservaddr));
	proxyservaddr.sin_family = AF_INET;
	inet_pton(AF_INET,proxy_server_ip,&(proxyservaddr.sin_addr));
	proxyservaddr.sin_port=htons(port);
	
	if ((bind(listenfd, (struct sockaddr *) &proxyservaddr, sizeof(proxyservaddr)))<0)
	{
		perror("bind error");
		return 0;
	}

	if ((listen(listenfd,10))<0)
	{
		perror("listen error");
		return 0;
	}

	int maxfd = listenfd;
	fd_set allset, rset;
	FD_ZERO(&allset);
	FD_ZERO(&rset);
	FD_SET(listenfd, &allset);

	for (;;)
	{
		rset = allset;
		int nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		for (int i=0; i<=maxfd; i++) // we check all the socket descriptors for data
		{
			if (FD_ISSET(i,&rset))//if there is a descriptor with connect
			{
				if (i==listenfd)//we got a new connection
				{
					handle_new_connection(listenfd, client_addr,allset,maxfd);

				}
				else// we got data from an existing connection
				{
					got_data(i, allset, maxfd);
					
				}
			}
		}
		
	}

	return 0;

}

