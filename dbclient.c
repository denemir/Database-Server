#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#include "msg.h"

#define MAX_NAME_LENGTH 128

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		printf("Usage: %s dnsname port\n", argv[0]);
		return 1;
	}
	
	char *dnsname = argv[1];
	int port = atoi(argv[2]);
	char port_str[6];
	sprintf(port_str, "%d", port);

#define SERVER_IP dnsname
#define PORT port

	char name[MAX_NAME_LENGTH];
	char buffer[1024];
	uint32_t id;
	struct msg req, resp;

	struct addrinfo hints, *result;
        int status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(dnsname,port_str, &hints, &result)) != 0)
	{
	perror("Error with addrinfo");
	exit(EXIT_FAILURE);
	}
	struct sockaddr_in *ipv4 = (struct sockaddr_in*) result->ai_addr;					 

	int client = socket(AF_INET, SOCK_STREAM, 0);
	if(client == -1)
	{
		perror("Failed to create socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	char ipv4_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipv4->sin_addr, ipv4_str, INET_ADDRSTRLEN);

	if(inet_pton(AF_INET, ipv4_str, &server_addr.sin_addr) <= 0)
	{
		perror("inet_pton");
		exit(EXIT_FAILURE);
	}

	if(connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("Connection failure");
		exit(EXIT_FAILURE);
	}


	printf("Successful connection to server!\n\n");	
	int choice = -1;
	//input loop
	while(choice != 0)
	{
	printf("Enter your choice (1 to put, 2 to get, 0 to quit): ");
	if(fgets(buffer, sizeof(buffer), stdin) == NULL)
	{
		perror("Error inputting choice");
		exit(EXIT_FAILURE);
	}
	choice = atoi(buffer);	
	switch(choice)
	{
		case 0:
		req.type = 0;
		if((send(client, &req, sizeof(req), 0) == -1))
		{
			perror("Client disconnect error");
			exit(EXIT_FAILURE);
		}
		close(client);
	
		return 0;
		break;

		case 1:
		printf("Enter the name: ");
		if(fgets(name, sizeof(name), stdin) == NULL)
		{
			perror("Error inputting name");
			exit(EXIT_FAILURE);
		}
		name[strcspn(name, "\n")] = 0; //removing newline

		printf("Enter the ID: ");
		if(fgets(buffer, sizeof(buffer), stdin) == NULL)
		{
			perror("Error inputting ID");
			exit(EXIT_FAILURE);
		}
		id = atoi(buffer);

		req.type = 1;
		strncpy(req.rd.name, name, sizeof(req.rd.name));
		req.rd.id = id;
		if(send(client, &req, sizeof(req), 0) == -1)
		{
			perror("Error sending data to server");
			exit(EXIT_FAILURE);
		}
		
		if(recv(client, &resp, sizeof(resp), 0) == -1)
		{
			perror("Error receiving response from server");
			exit(EXIT_FAILURE);
		}
		printf("Put success\n");
		break;

		case 2:
		printf("Enter ID: ");
		if(fgets(buffer, sizeof(buffer), stdin) == NULL)
		{
			perror("Error inputting ID");
			exit(EXIT_FAILURE);
		}
		id = atoi(buffer);

		req.type = 2;
		req.rd.id = id;
		if(send(client, &req, sizeof(req), 0) == -1)
		{
			perror("Error sending request to server!");
			exit(EXIT_FAILURE);
		}

		if(recv(client, &resp, sizeof(resp), 0) == -1)
		{
			perror("Error receiving response from server");
			exit(EXIT_FAILURE);
		}

		switch(resp.type)
		{
			case 4: //success
			printf("Name: %s\n", resp.rd.name);
			printf("ID: %u\n", resp.rd.id);
			break;
			case 5: //failure
			perror("Unsuccessful response from server");
			break;
			default:
			printf("Unknown Response\n");
			break;
		}


		break;
	}

	}

	close(client);
}
