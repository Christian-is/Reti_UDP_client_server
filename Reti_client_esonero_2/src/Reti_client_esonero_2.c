/*
 ============================================================================
 Name        : Reti_server_esonero_2.c
 Author      : Christian Giacovazzo
 Description : secondo esonero di "Reti di calcolatori 2021/2022" - client udp
 ============================================================================
 */

#if defined WIN32
	#include <winsock.h>
#else
	#define closesocket close
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

//print an error message
void errorhandler(char *errorMessage) {
	printf("%s", errorMessage);
}

//clear socket if WIN32
void clearwinsock() {
	#if defined WIN32
	WSACleanup();
	#endif
}

//sets message structure to default value
void clearMsg(message* msg){
	msg->operation = '0'; //default
	msg->operatorA = 0; //default
	msg->operatorB = 0; //default
}

//return 1 if str is not a number, 0 otherwise
int isNotNumber(const char* str){
	int len = strlen(str);
	char number[len+1];
	strcpy(number, str);
	int i = 0;

	if(number[i] == '+' || number[i] == '-'){
		i++;
	}
	while(number[i] != '\0'){
		if(number[i] < '0' || number[i] > '9'){
			return 1;
		}
		i++;
	}

	return 0;
}

//parse str into tokens divided by 'space', and saves them in msg. If invalid str, return default msg
message parseString(char* str) {
	message msg;
	char *token1; //token for the operation
	char *token2; //token for the first operator
	char *token3; //token for the second operator
	char *tokenNull = NULL; //flag error token. If not NULL, there is an error
	clearMsg(&msg);

	if(str[0] != '\n'){ //if str is new line, error
		str[strcspn(str, "\n")] = 0; //remove \n from str
		//extract tokens
		token1 = strtok(str, " ");
		token2 = strtok(NULL, " ");
		token3 = strtok(NULL, " ");
		tokenNull = strtok(NULL, " "); //if this token is not null, input error (too many operators)

		if(strcmp(token1, "=") == 0 && token2 == NULL){ //quit flag
			msg.operation = *token1;
			return msg;
		}

		if(tokenNull != NULL || token1 == NULL || token2 == NULL || token3 == NULL){ //input error
			clearMsg(&msg);
			return msg;
		}

		//assign tokens if valid
		//first token
		if(strcmp(token1, "+") != 0 && strcmp(token1, "-") != 0 && strcmp(token1, "x") != 0 && strcmp(token1, "/") != 0){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operation = *token1;
		}

		//second token
		if(isNotNumber(token2)){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operatorA = strtol(token2, (char **)NULL, 10);
		}

		//third token
		if(isNotNumber(token3)){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operatorB = strtol(token3, (char **)NULL, 10);
		}

		return msg;
	}else{
		clearMsg(&msg);
		return msg;
	}
}

//parse <str = 'abc:123'> to <name = 'abc'> and <port = '123'>
void parseNameAndPort(const char* str, char* name, int* port){
	int i = 0;
	//find the name
	while(str[i] != ':'){
		if(str[i] == '\0'){ // ':' is not present, there is no port number
			return;
		}
		i++;
	}
	//if im here, the name is valid
	strncpy(name, str, i);
	name[i+1] = '\0';
	i++; //jump over ':'

	//now find the port number
	char portString[BUFFERSIZE];
	strcpy(portString, str + i);
	*port = atoi(portString);
}


//calculator function
void calculatorClient(int c_socket, struct sockaddr_in sad){
	int LEN = 50;
	char input[LEN]; //string input from user
	int errorFlag = 0; //1 if invalid input, 0 otherwise
	message msg; //message struct that must be sent to the server

	printf("\nInsert one of the following operation in brackets { + - x / } followed by two integer (operators).\n");
	printf("Each value must be separated by a space (Example: + 23 45).\n");
	printf("If wrong input format is used, retry on new line. Insert { = } to quit.\n");

	while(1){
		clearMsg(&msg); //set msg to default
		//get input (single string input)
		printf("Insert new input: ");
		do{
			errorFlag = 0;
			fflush(stdout); //may not be needed for linux
			fflush(stdin); //may not be needed for linux
			fgets(input, LEN, stdin); //need this because 'space' is a valid char
			msg = parseString(input);
			if(msg.operation == '=') {
				//quit flag
				free(input); //free memory
				return;
			}
			if(msg.operation == '0'){ //if msg is in default state, errorFlag = 1
				errorFlag = 1;
				errorhandler("ERROR. Insert valid input: ");
			}
		}while(errorFlag);

		//send message
		msg.operatorA = htonl(msg.operatorA);
		msg.operatorB = htonl(msg.operatorB);
		sendto(c_socket, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&sad, sizeof(sad));

		//receive message
		struct sockaddr_in fromAddr;
		unsigned int fromSize;
		fromSize = sizeof(fromAddr);
		char buffer[BUFFERSIZE]; //result message
		int respStringLen = recvfrom(c_socket, buffer, BUFFERSIZE, 0, (struct sockaddr*)&fromAddr, &fromSize);
		if (sad.sin_addr.s_addr != fromAddr.sin_addr.s_addr) { //check source of message; if wrong source, quit
			errorhandler("Error: received a packet from unknown source.\n");
			return;
		}
		buffer[respStringLen] = '\0';

		//print result
		struct hostent* serverIP;
		serverIP = gethostbyaddr((char*) &sad.sin_addr.s_addr, 4, AF_INET); //retrive canonic server name
		struct in_addr* IP = (struct in_addr*) serverIP->h_addr_list[0];
		printf("Result from server %s, ip %s: %s\n", serverIP->h_name, inet_ntoa(*IP), buffer);
		memset(buffer, 0, sizeof(buffer));
	}
}

//build server address
struct sockaddr_in buildServerAddr(char* serverNameAndPort) {
	char ip_address[BUFFERSIZE];
	char canonicName[BUFFERSIZE];
	int port;

	//get server from argv[] input
	if(serverNameAndPort == NULL){
		printf("Server address not found. Using default name %s, default ip %s, default port %d\n", PROTONAME, PROTOIP, PROTOPORT);
		port = PROTOPORT; //set default port
		strcpy(ip_address, PROTOIP); //set default ip
		strcpy(canonicName, PROTONAME); //set default name server
	}else{
		parseNameAndPort(serverNameAndPort, canonicName, &port);
		printf("argv[1] = %s. Using server name %s, port %d\n", serverNameAndPort, canonicName, port);
	}

	//get server from scanf input
/*	//printf("Insert name of the server host: "); //request canonic name server
	//scanf("%s",canonicName);
	//printf("Insert server port number: ");  //request port
	//scanf("%d",&port);
*/

	struct hostent* serverIP;
	serverIP = gethostbyname(canonicName);  //request IP from DNS
	if (serverIP == NULL) {
		errorhandler("Error DNS request...\n");
		system("pause");
		exit(EXIT_FAILURE);
	}
	if(port <= 0) {
		errorhandler("Error: invalid port...\n");
		system("pause");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serverAddr;  //build server address
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	memcpy(&serverAddr.sin_addr, serverIP->h_addr_list[0], serverIP->h_length);
	serverAddr.sin_port = htons(port);
	return serverAddr;
}

int main(int argc, char *argv []) {

	printf("Client started.\n");
	printf("(NOTE: Default server name is %s, default server IP is %s , default server port is %d)\n", PROTONAME, PROTOIP, PROTOPORT);


	#if defined WIN32
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2 ,2), &wsa_data);

	if (result != 0) {
		errorhandler("error at WSASturtup\n");
		return -1;
	}
	#endif

	//SOCKET CREATION
	int c_socket;
	c_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (c_socket < 0) {
		errorhandler("socket creation failed.\n");
		closesocket(c_socket);
		clearwinsock();
		return -1;
	}

	// SERVER ADDRESS CONSTRUCTION
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad = buildServerAddr(argv[1]);

	//client request
	calculatorClient(c_socket, sad);

	// CLOSE CONNECTION
	closesocket(c_socket);
	clearwinsock();

	printf(" \n");
	system("pause");
	return(0);
}
