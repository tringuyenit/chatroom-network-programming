#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "utils.h"

#define LENGTH 2048
char msg[LENGTH];
// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char password[32];

void catch_ctrl_c_and_exit(int sig)
{
	flag = 1;
}

// send msg handler
// if quit, send last msg and quit system
void send_msg_handler()
{
	char message[LENGTH] = {}, receiver[LENGTH];

	// clear the buffer
	msg[0] = '\0';
	receiver[0] = '\0';

	while (1)
	{
		str_overwrite_stdout();
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "quit") == 0)
		{
			send(sockfd, message, strlen(message), 0);
			break;
		}

		send(sockfd, message, strlen(message), 0);
		bzero(message, LENGTH);
	}
	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler()
{
	char receiveMess[LENGTH] = {};
	while (1)
	{
		int receive = recv(sockfd, receiveMess, LENGTH, 0);
		//printf(" receive:%s", receiveMess);
		if (receive > 0)
		{
			printf("%s", receiveMess);
			if (strlen(receiveMess) != receive)
				flag = 1;
			str_overwrite_stdout();
		}
		else if (receive <= 0)
		{
			flag = 1;
			break;
		}
		memset(receiveMess, 0, sizeof(receiveMess));
	}
}

// get username and password
void getUserInfor()
{
	memset(name, 0, 32);
	memset(password, 0, 32);
	printf("Please enter your name: ");
	scanf("%s", name);
	str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Name must be less than 30 and more than 2 characters.\n");
		getUserInfor();
	}
	printf("Please enter your password: ");
	scanf("%s", password);
	str_trim_lf(password, strlen(password));
	//printf("%s %s", name, password);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("Usage: ./client <server-ip> <port>\n");
		return EXIT_FAILURE;
	}

	int port = atoi(argv[2]);

	signal(SIGINT, catch_ctrl_c_and_exit);
	int option;
	char message[67] = {0};
	char response[LENGTH];
	struct sockaddr_in server_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(port);
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1)
	{
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	do
	{
		memset(response, 0, LENGTH);
		// get option from client
		// case 1 and 2, get user infor
		// case 3 send quit msg and quit system
		// loop until logged in successful or quit
		option = getOption();
		if (option == 3)
		{
			send(sockfd, "3\0", 3, 0);
			close(sockfd);
			return 0;
		}
		getUserInfor();
		memset(message, 0, 67);
		if (option == 1)
			strcat(message, "1 ");
		else
			strcat(message, "2 ");
		strcat(message, name);
		strcat(message, " ");
		strcat(message, password);

		// Send name
		send(sockfd, message, 67, 0);
		//	printf("send to server: %s\n", message);

		if (recv(sockfd, response, LENGTH, 0) < 0)
			printf("Error received\n");
		printf("Noti:%s\n", response);

	} while (!(strstr(response, "success") && option == 1));

	if (option == 1)
	{
		printf("=== WELCOME TO THE CHATROOM ===\n(type \"help\" for usage)\n");

		// thread for sending msg to server
		pthread_t send_msg_thread;
		if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
		{
			printf("ERROR: pthread\n");
			return EXIT_FAILURE;
		}

		// thread for receiving msg from server
		pthread_t recv_msg_thread;
		if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
		{
			printf("ERROR: pthread\n");
			return EXIT_FAILURE;
		}

		while (1)
		{
			if (flag)
			{
				printf("\nBye\n");
				break;
			}
		}

		close(sockfd);
	}

	return EXIT_SUCCESS;
}