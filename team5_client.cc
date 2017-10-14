
#include "team5_client.h"

void send_http_request_message(int sockfd, char* URL)
{
	
	char host[256];
	char dir[512];
	int x=0;
	while (URL[x]!='/')
	{
		host[x] = URL[x];
		x++;
	}
	host[x] = '\0';
	int y=0;
	for (y=0; y<strlen(URL); y++)
	{
		dir[y] = URL[y+x];
	}
	dir[y]='\0';
	cout<<host<<endl;
	cout<<dir<<endl;

	//buffer_size = strlen(URL) + 1;
	char buffer[1024];
	memset(buffer, 0, 1024);
	strcpy(buffer, "GET ");
	strcat(buffer, dir);
	strcat(buffer, " HTTP/1.0\r\n");
	strcat(buffer, "Host: ");
	strcat(buffer,host);
	strcat(buffer,"\r\nConnection: close");	
	strcat(buffer,"\r\n\r\n");
	cout<<buffer<<endl;
	int bytes_sent = send(sockfd, buffer, strlen(buffer)+1, 0);
}

void got_data(int descriptor)
{
	char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);
	int nbytes = recv(descriptor, buffer, BUFF_SIZE, 0);
	cout<<nbytes<<endl;
	std::string str_buffer(buffer);
	std::remove(str_buffer.begin(),str_buffer.end(),'\r');
	std::string str;
	std::stringstream stream(str_buffer);
	getline(stream,str);
	
	if (str.substr(9,3) == "200")
	{
		cout<<"got the web page!"<<endl;
		str.clear();
		std::map<std::string,std::string> response_map;
		std::string::size_type index;
		while(getline(stream, str))
		{
			if (str.length()==0)
				break;
			index = str.find(':', 0);
			response_map.insert(std::pair<std::string,std::string>(str.substr(0,index),str.substr(index+2)));
			str.clear();
		
		}
		str.clear();
		//std::map<std::string,std::string>::iterator i = response_map.begin();	
		//for (i=response_map.begin(); i!=response_map.end(); i++)
		//	cout<<i->first<<" "<<i->second<<endl;
		fstream(myfile);	
		myfile.open("output_page",ios::out);
	
		while(getline(stream, str))
		{
			//cout<<str<<endl;
			myfile<<str;
			str.clear();
		
		}
		myfile.close();
	}
	else if (str.substr(9,3) == "301")
		cout<<"moved permanently"<<endl;
	else if (str.substr(9,3) == "302")
		cout<<"found"<<endl;
	else if (str.substr(9,3) == "400")
		cout<<"bad request"<<endl;
	else if (str.substr(9,3) == "401")
		cout<<"unauthorized"<<endl;
	else if (str.substr(9,3) == "403")
		cout<<"forbidden"<<endl;
	else if (str.substr(9,3) == "404")
		cout<<"not found"<<endl;
	else if (str.substr(9,3) == "500")
		cout<<"internal server error"<<endl;	
	else if (str.substr(9,3) == "501")
		cout<<"not implemented"<<endl;
	else if (str.substr(9,3) == "502")
		cout<<"bad gateway"<<endl;
	else if (str.substr(9,3) == "503")
		cout<<"service unavailable"<<endl;
	
}

int main(int argc, char** argv)
{
	if (argc!=4)
	{
		cout<<"Please enter 3 arguments"<<endl;
		cout<<"quitting ..."<<endl;		
		return 0;
	}
	char temp_URL[512];
	char URL[512];	
	char server_ip[50];
	char proxy_server_port_string[50];
	
	unsigned short int proxy_server_port;	
	strcpy(server_ip,argv[1]);
	strcpy(proxy_server_port_string,argv[2]);
	strcpy(temp_URL,argv[3]);
	proxy_server_port = atoi(proxy_server_port_string);
	
	if (temp_URL[0] == 'h' && temp_URL[1] == 't' && temp_URL[2] == 't' && temp_URL[3] == 'p' && temp_URL[4] == ':' && temp_URL[5] == '/' && temp_URL[6] == '/')
	{
		int i;
		for (i=0; i<strlen(temp_URL)-7; i++)
		{
			URL[i] = temp_URL[i+7];
		}
		URL[i]='\0';
	}

	else 
	{
		int i;
		for (i=0; i<strlen(temp_URL); i++)
		{
			URL[i] = temp_URL[i];
		}
		URL[i]='\0';
	}
		

	int sockfd;
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error");
		return 0;
	}
	struct sockaddr_in proxy_server_addr;
	bzero(&proxy_server_addr,sizeof(proxy_server_addr));
	proxy_server_addr.sin_family = AF_INET;
	proxy_server_addr.sin_port = htons(proxy_server_port);
	inet_pton(AF_INET,server_ip,&(proxy_server_addr.sin_addr));
	if ((connect(sockfd,(struct sockaddr*) &proxy_server_addr, sizeof(proxy_server_addr)))<0)
	{
		perror("connect error");
		return 0;
	}
	
	
	
	send_http_request_message(sockfd, URL);

	got_data(sockfd);
	close(sockfd);
	return 0;
}


