/*
 ============================================================================
 Name        : Reti_server_esonero_2.c
 Author      : Christian Giacovazzo
 Description : secondo esonero di "Reti di calcolatori 2021/2022" - server udp
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
#include <math.h>
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

//addition function
long signed int add(long signed int a, long signed int b) {
	return a + b;
}

//subtraction function
long signed int sub(long signed int a, long signed int b) {
	return a - b;
}

//multiplication function
long signed int mult(long signed int a, long signed int b) {
	return a * b;
}

//division function
float division(long signed int a, long signed int b) {
	float c = (float)a / (float)b;
	return c;
}

//sets message structure to default value
void clearMsg(message* msg){
	msg->operation = '0'; //default
	msg->operatorA = 0; //default
	msg->operatorB = 0; //default
}

//build operation string
void buildOperationString(char *operationString, char *buffer, message msg){
	char temp[BUFFERSIZE];
	strcat(operationString, itoa(msg.operatorA, temp, 10));
	strcat(operationString, " ");
	temp[0] = msg.operation;
	temp[1] = '\0';
	strcat(operationString, temp);
	strcat(operationString, " ");
	strcat(operationString, itoa(msg.operatorB, temp, 10));
	strcat(operationString, " = ");
	strcat(operationString, buffer);
}

//calculator function
void calculatorServer(int client, struct sockaddr_in cad) {
	message msg;
	long signed int result; //operation result
	float divResult; //division result
	int resultNaN = 0; //NaN flag: 1 yes, 0 no

	clearMsg(&msg); //clear struct and set default values, just to be safe

	//receive message
	unsigned int cliAddrLen = sizeof(cad);
	recvfrom(client, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&cad, &cliAddrLen);
	msg.operatorA = ntohl(msg.operatorA);
	msg.operatorB = ntohl(msg.operatorB);

	//retrive client information and print his request
	struct hostent* clientIP;
	clientIP = gethostbyaddr((char*) &cad.sin_addr.s_addr, 4, AF_INET); //retrive canonic client name
	struct in_addr* IP = (struct in_addr*) clientIP->h_addr_list[0];
	printf("Request operation '%c %ld %ld' from client %s, ip %s\n", msg.operation, msg.operatorA, msg.operatorB, clientIP->h_name, inet_ntoa(*IP));

	//compute request
	if(msg.operation == '+') {
		//addition
		result = add(msg.operatorA, msg.operatorB);
	}else if(msg.operation == '-') {
		//subtraction
		result = sub(msg.operatorA, msg.operatorB);
	}else if(msg.operation == 'x') {
		//multiplication
		result = mult(msg.operatorA, msg.operatorB);
	}else if(msg.operation == '/') {
		//division
		if(msg.operatorB != 0){
			divResult = division(msg.operatorA, msg.operatorB);
		}else{
			resultNaN = 1;
		}
	}

	//send result message in string form
	char fullOperationString[BUFFERSIZE]; //operation in string form
	char resultString[BUFFERSIZE]; //result of the operation in string form
	if(msg.operation == '/' && resultNaN != 1){
		sprintf(resultString, "%g", divResult); //saves float to a string
	}else if(msg.operation == '/' && resultNaN == 1){
		strcpy(resultString, "nan");
	}else{
		sprintf(resultString, "%ld", result); //saves long int to a string
	}
	memset(fullOperationString, 0, sizeof(fullOperationString)); //clear string
	buildOperationString(fullOperationString, resultString, msg); //build message that will be sent to client
	int bufferLenght = strlen(fullOperationString);
	if(bufferLenght > BUFFERSIZE){
		errorhandler("buffer too long\n");
	}
	sendto(client, fullOperationString, bufferLenght, 0, (struct sockaddr *)&cad, sizeof(cad)); //send message to client
	memset(fullOperationString, 0, sizeof(fullOperationString)); //clear string
	printf("Client %s, ip %s served\n", clientIP->h_name, inet_ntoa(*IP));

}

int main(int argc, char *argv []) {
	//selecting a port number for the server (use default PROTOPORT)
	int port = PROTOPORT;

	//WSAStartup only for windows
	#if defined WIN32
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != 0) {
		errorhandler("Error at WSAStartup().\n");
		return -1;
	}
	#endif

	//SOCKET CREATION
	int my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (my_socket < 0) {
		errorhandler("socket creation failed.\n");
		clearwinsock();
		return -1;
	}

	//ASSIGNING AN ADDRESS TO THE SOCKET
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad)); //clear space
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(PROTOIP); //loopback IP (local);
	sad.sin_port = htons(port);
	if (bind(my_socket, (struct sockaddr *)&sad, sizeof(sad))< 0) {
		errorhandler("bind() failed.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	printf("UDP server started. Waiting for requests...\n");

	//COMPUTE REQUEST FROM CLIENT
	struct sockaddr_in cad; // structure for the client address
	while (1) { //computing request from client
		calculatorServer(my_socket, cad);
	}

	system("pause");
	return 0;
}
