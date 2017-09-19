/*
 * Author: Thomas Buteau
 * Program: OTP - otp_enc_d
 * Class: CS344-400
 * Date: 8-18-17
 * Decription: The otp_enc_d program runs in the background on the port provided as an argument when the program
 * is run. This program will listen on the port provided for communication from the otp_enc program and when this
 * occurs generate a child process to handle the transaction while continuing to listen for more connections.
 * The child process verifies that it is otp_enc that it is communicating with and then accepts the message and
 * key, encodes the message, then writes back the encoded message to otp_enc. Note that this program supports
 * up to five (5) concurrent connections.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues 

/*
 * The encrypt function takes in two strings as arguments: a message and a key. It uses the key to 
 * transform the message into a non-readable string and returns said string. Note that the transformation
 * is not case-sensitive.
 */
char* encrypt(char* message, char* key) {
	char* range = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char* encrypted = (char*) malloc(sizeof(message)-1); 
	//printf("Server: In encrypt function encrypted is malloc'ed to size %d\n", sizeof(message));
	memset(encrypted, '\0', sizeof(encrypted));
	int* mInt = (int*) malloc(strlen(message) * sizeof(int));
	int* kInt = (int*) malloc(strlen(key) * sizeof(int));
	int x, y;
	//convert message and key to integer arrays
	for(x=0;x<strlen(message);x++) {
		for(y=0;y<strlen(range);y++) {
			if(toupper(message[x]) == range[y]) {
				mInt[x] = y;
			}
			if(toupper(key[x]) == range[y]) {
				kInt[x] = y;
			}
		}
	}
	
	//modify message with key
	for(x=0;x<strlen(message);x++) {
		mInt[x] += kInt[x];
		mInt[x] = mInt[x] % 27; //27 is used because there are 26 letters + space total options
	}

	//write encrypted
	for(x=0;x<strlen(message);x++) {
		encrypted[x] = range[mInt[x]];
	}

	if(strlen(encrypted) > strlen(message)) {
		encrypted[strlen(message)+1] = '\0';
	}

	return encrypted;
}

int main(int argc, char *argv[])
{
        int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
        char buffer[70000];
        struct sockaddr_in serverAddress, clientAddress;
	pid_t processing_pid;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

        // Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
        
        // Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");
        
        // Enable the socket to begin listening
        if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
        	error("ERROR on binding");
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	        
	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");
	
		processing_pid = fork();
		switch(processing_pid) {
			//error with fork
			case -1: {error("Fork error");break;}
			//child process runs
			case 0: {
					// Get the message from the client and display it
					memset(buffer, '\0', sizeof(buffer));
					charsRead = recv(establishedConnectionFD, buffer, 70000, 0); // Read the client's message from the socket
					if (charsRead < 0) error("ERROR reading from socket");
					char* message = (char*) malloc(70000 * sizeof(char*));
					memset(message, '\0', sizeof(message));
					strcpy(message, buffer); 
					
					memset(buffer, '\0', sizeof(buffer));
					charsRead = recv(establishedConnectionFD, buffer, 70000, 0); // Read in the encryption key from the socket
					if (charsRead < 0) error("Error reading from socket");
					char* key = (char*) malloc(70000 * sizeof(char*));
					memset(key, '\0', sizeof(key));
					strcpy(key, buffer);
					
					char* encMSG = (char*) malloc(70000 * sizeof(char*));
					memset(encMSG, '\0', sizeof(encMSG));
					encMSG = encrypt(message, key); //writes the encrypted message to encMSG
					charsRead = send(establishedConnectionFD, encMSG, strlen(encMSG), 0); // Send encrypted message back
					if (charsRead < 0) error("ERROR writing to socket");
					
					close(establishedConnectionFD); // Close the existing socket which is connected to the client
					exit(0); 
				}
			default: { } //intentionally left blank
		}
	}  
        close(listenSocketFD); // Close the listening socket
	return 0;
}
