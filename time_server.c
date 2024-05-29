#if defined (_WIN32)
#ifndef _WIN32_WINNT
#define _WIN_32_WINNT 0x0600
#endif 

#include<winsock2.h>
#include<ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")



#else

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
#include<errno.h>
#endif

//Standard c headers 
#include<stdio.h>
#include<string.h>
#include<time.h>

//Here define macros that abstract out the differenes between
//Berekely sockets and winsocks
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else

#define ISVALIDSOCKET(s) ((s)>=0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

int main()
{
//First we define initialize Winsock if we are compiling in windows 
# if defined(_WIN32)
	WSADATA d;
	if(WSAStartup(MAKEWORD(2,2), &d))
	{
		fprintf(stderr,"(WIndows build) Failed to initialize");
		return 1;
	}
#endif


//Now we figure out the local address our webserver should bind to 
printf("configuring local address ...\n");
struct addrinfo hints;
//Setting hints addrinfo object to all 0's initially (clearing out mememory)
memset(&hints, 0, sizeof(hints));

//Using this to create a struct of info to pass into getaddrinfo()
hints.ai_family=AF_INET;
hints.ai_socktype= SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

struct addrinfo *bind_address;
/*
 *We use getaddrinfo() to fill in a struct addrinfo structure with the needed
 information. In this case we set hints ai_family to AF_NET in order to sepcifiy that we're looking
 for an IPv4 address



 Next we set ai_soctype = SOCK_STREAM this indicates we are using Tranmission Control Protocol 
 (SOCK_DGRAM woul dbe used for User Datagram protocol)

 Lastly, setting ai_flags to passive lets getaddrinfo() know that we want it to bind to the
wildcard address: we want getaddrinfo() to to set up the address, so we listen on any 
available network interface
 * 
 * */


/*
 In this case getaddrinfo() is used for generating an address that suitable for bind().

 We do this by making the first parameter NULL and having the hints.ai_flags = AI_PASSIVE
 The second parameter is the port we want to bind on. 
 * */

getaddrinfo(0, "8080", &hints, &bind_address);
//Now to create the socket
printf("Creating Socket...\n");
//The listening socket for our program
SOCKET socket_listen;

/*socket() function takes three paremeters 
 * 1) socket familiy, 
 * 2) socket type, 
 * 3) socket protocol*/
socket_listen = socket(bind_address-> ai_family, 
		       bind_address-> ai_socktype,
		       bind_address-> ai_protocol);
//Check that the call to socket was successful
if(!ISVALIDSOCKET(socket_listen))
{
	fprintf(stderr, "socket failed. (%d)\n", GETSOCKETERRNO());
	return 1;
}
printf("Binding socket to local address....\n ");
//Check if the binding failed.
if(bind(socket_listen,
	bind_address -> ai_addr,
	bind_address -> ai_addrlen 
	))
{
fprintf(stderr, "bind() failed. (%d)", GETSOCKETERRNO());
return 1;
}
//Note: bind() returns 0 on sucess and non-zeror on failure


/*After we have bond the address to struct addrinfo bind_adresswe call freeaddrinfo() 
 * to release the adress memory*/

//Now we can listen can cause this socket ot listen for connections
printf("Listening... \n");
if(listen(socket_listen, 10)<0)
{
fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
return 1; 
}
//Second argument of listen() tells listen how many connections it is allowed to queue up to
printf("Waiting for connection... \n");
//Holds client socket information
struct sockaddr_storage client_address;
socklen_t client_len = sizeof(client_address);

//Gather information about connected client
/*
 *Accept has a few functions
 1) when it is called, it will block your program until a new connection is made
 2) When a newconnection is made, accept will make a new socket for it
 3) Original socket continue to listen for connections but the new sockets 
 returned by accept() can be used to send and receive data over the newly established connection
 4) accept also fills in address info of the client that connected 
 * */

/*Before calling accept() we declaare a strut sockaddr_storage vairbal eto store
 * the addres info fo teh connecting client (guaranteed to be large enough to store 
 * the largets supported address on your system)
 * We also tell accept the size of the address that we're passing in 
 *
 * */
SOCKET socket_client = accept(socket_listen,
			      (struct sockaddr*) &client_address, 
			      &client_len);
//Error handling for client socket address
if(!ISVALIDSOCKET(socket_client))
{
fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
return 1;
}


printf("Client is connected.. \n");
//Logging network connections
char address_buffer[100];
getnameinfo((struct sockaddr*) &client_address, 
	    client_len,
	    address_buffer, 
	    sizeof(address_buffer),
	    0,0,
	    NI_NUMERICHOST 
		);
/*
 *getnameinfo() takes the clients address and address length.
  We then pass in an outputbuffer and buffer length
  this is the buffer that getnameinfo() writes is hostname output to. 
  The next two arguments specify second buffer and its length 
  getnameinfo() outpouts the service name into this buffer. (we passed in 0 to these two parameters)
  Then we specify the NI_NUMERICHOST flag which means we want to see 
  hostname as an IP address.
 * */



/* As we are programming a webserver, we expect the client to send us an HTTP Request. 
 *
 * We read this request using the recv() function 
 * */

printf("Reading Request...\n");
char request[1024];
/*recv() returns the number of bytes that were received. 
 * If nothing is received, it blocks until something has been. 
 * If connection has been terminated by client, returns 0 or -1 dependign on circumstances
 * In production you should always check that recv()>0
 * Last parameter is for flags but we set that to 0*/

int bytes_received =  recv(socket_client, request, 1024, 0);
printf("Received %d bytes.\n", bytes_received);
//printing the request received
//No guarantee that bytes received from recv() are null terminated 
printf("%.*s", bytes_received, request);


//Now to send our response back!
printf("Sending response....\n");
const char* response =
"HTTP/1.1 200 OK"
"Connection: close\r\n"
"Content-type:text/plain\r\n\r\n"
"Local time is: ";
int bytes_sent = send(socket_client, response, strlen(response), 0);
printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));


//After sending the first part of our HTTP response, we can now send the time
time_t timer;
time(&timer);
char* time_msg = ctime(&timer);
bytes_sent= send(socket_client, time_msg, strlen(time_msg), 0);
printf("Sent %d out of %d bytes", bytes_sent, (int)strlen(time_msg));

//We then close the connection to indicate to our browser that we set all of the data
printf("Closing connection...\n");
CLOSESOCKET(socket_client);
printf("closing listening socket... \n");
CLOSESOCKET(socket_listen);

#if defined(_WIN32) 
	WASCleanup();
#endif 

printf("Finished.\n");
return 0;
}
