/*
 * protocol.h
 *
 *  Created on: 11 dec 2021
 *      Author: cgiac
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

//default name server
#define PROTONAME "localhost"
//default IP address
#define PROTOIP "127.0.0.1"
//default protocol port number
#define PROTOPORT 60000
//max buffer lenght
#define BUFFERSIZE 512

//struct of a message
typedef struct{
	char operation; //+ - x / or =
	long signed int operatorA; //signed integer
	long signed int operatorB; //signed integer
}message;


#endif /* PROTOCOL_H_ */
