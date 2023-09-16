#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "msg.h"

#define PORT port

#define MAX_RECORD_COUNT 10000000

int server_quit = 0; //if 1 then quit

struct record db[MAX_RECORD_COUNT];
int num_records = 0;

//if client enters put
void put(char *name, uint32_t id)
{
	if(id < MAX_RECORD_COUNT)
	{
		struct record *r = &db[id];
		strncpy(r->name, name, 100);
		r->id = id;
		FILE *dbf = fopen("database.txt", "a");
		if(dbf == NULL)
		{
			perror("Failed to open database file\n");
		}
		else fprintf(dbf, "%s %d\n", r->name, r->id);

		fclose(dbf);
	}	
}

struct record def_record = { .name = {'\0'}, .id = -1 };
//if client enters get
struct record *get(uint32_t id)
{

	FILE *dbf = fopen("database.txt", "r");
	if(dbf == NULL)
	{
		perror("Failed to open database file\n");
		return &def_record;
	}

	struct record *r = &db[id];
	char line[128];
	while(fgets(line, sizeof(line), dbf) != NULL)
	{
	int re = sscanf(line, "%s %u", r->name, &(r->id));
		if(re == 2 && r->id == id)
		{
			fclose(dbf);
			printf("%s %d\n", r->name, r->id);
			return r;
		}
		
	}

	
	fclose(dbf);
	//if record not found
	printf("Record not found!\n");
	return &def_record;
	}

void *handler(void *arg)
{
	int client = *(int*)arg;
	struct msg req, resp; //message request and response
	
	//printf("thread created successfully\n");
	//handle
	while(1)
	{
		int bytes = read(client, &req, sizeof(req));
		if(bytes < 0)
		{
			perror("Error reading request\n");
			//exit(EXIT_FAILURE);

		} else if (bytes == 0) {
			printf("Client ended connection\n");
			close(client);
			pthread_exit(NULL);
			//exit(EXIT_FAILURE);
			continue;

		} else printf("Server: got message: '%d', %s, %u\n",req.type, req.rd.name, req.rd.id);

	//printf("Request received! \n");

	struct record *r;
		switch(req.type)
		{
		case 0: //quit
			printf("Host quit\n");
			//server_quit = 1;
			close(client);
			pthread_exit(NULL);
			//exit(EXIT_FAILURE);
			break;
		case 1: //put
			put(req.rd.name, req.rd.id); //storing name & id
			resp.type = 1;
			printf("Successfully put %s, ID %u\n", req.rd.name, req.rd.id);
			if(send(client, &resp, sizeof(resp), 0) == -1)
			{
				perror("Failed to respond to client\n");
				//exit(EXIT_FAILURE);
			} else printf("Server: sent message - '%d', %s, %u\n", resp.type, resp.rd.name, resp.rd.id);
			break;

		case 2: //get
			r = get(req.rd.id);
			if(r->id != -1)
			{
				resp.type = 4;
				resp.rd = *r;
				printf("Successfully got %s, ID %u\n", r->name, r->id);
				if(send(client, &resp, sizeof(resp), 0) == -1)
				{
					perror("Failed to respond to client");
					//exit(EXIT_FAILURE);
				}
				else printf("Server: sent message: '%d', %s, %d\n",resp.type, resp.rd.name, resp.rd.id);
																
			}
			else {
				resp.type = 5;
				printf("Failed to get\n");
				if(send(client, &resp, sizeof(resp), 0) == -1)
				{
					perror("Failed to respond to client");
					//exit(EXIT_FAILURE);
				} else printf("Server: sent message - '%d', %s, %d\n", resp.type, resp.rd.name, resp.rd.id);
			}
			break;
			default:
			printf("something went wrong\n");
			resp.type = 5;
			break;
		}
	}

	close(client);

	pthread_exit(NULL);

}


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: %s port", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);

	int server, client, address_length;
	struct sockaddr_in server_address, client_address;

	server = socket(AF_INET, SOCK_STREAM, 0); //1.socket()
	if(server == -1) //create socket errror
	{
		perror("Failed to create server socket");
		exit(EXIT_FAILURE);
	}

	//2.bind()
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);
	if(bind(server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
	{
		perror("Failed to bind server socket");
		exit(EXIT_FAILURE);
	}

	//3.listen()
	if(listen(server, 5) == -1)
	{
		perror("Failure to listen to client");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d\n\n", PORT);
	

	while(server_quit != 1)
	{
		//4. accept()
		address_length = sizeof(client_address);
		client = accept(server, (struct sockaddr*)&client_address, (socklen_t*) &address_length);
		if(client == -1)
		{
			perror("Failed to accept client connection");
			continue;
		}

		printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

		//5. read/write()
		pthread_t thread_id;
		if(pthread_create(&thread_id, NULL, handler, &client) == -1)
		{
			perror("Failed to create thread");
			close(client);
		}
	}

	//6. close()
	close(server);
	printf("Server closed\n");

	return 0;



}
