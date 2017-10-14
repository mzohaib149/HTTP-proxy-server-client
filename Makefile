all: client proxy clean

client: team5_client.o
	g++ team5_client.o -o client

proxy: team5_proxy.o
	g++ team5_proxy.o -o proxy

team5_client.o: team5_client.cc team5_client.h team5_common.h
	g++ -c team5_client.cc

team5_proxy.o: team5_proxy.cc team5_proxy.h team5_common.h
	g++ -c team5_proxy.cc

clean: 
	rm *.o
