/*
 * Author: Thomas Buteau
 * Program: OTP - otp_dec
 * Class: CS344-400
 * Date: 8-18-17
 * Description: The otp_dec program takes the encrypted message and key given as arguments and sends them to
 * 		otp_dec_d as pointed to by the port number provided and then receives the decrypted message
 * 		and prints it out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

void sendData(int socketFD, char* message) {
	int charsWritten;
	charsWritten = send(socketFD, message, strlen(message), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(message)) printf("CLIENT: WARNING: Not all data written to socket!\n");
}
 
int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[70000];

	if (argc < 3) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]);
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
		error("CLIENT: ERROR connecting");
	}
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	FILE *rFile;
	rFile = fopen(argv[1], "r");
	if (rFile == NULL) printf("File was not opened.\n");
	fgets(buffer, sizeof(buffer), rFile);
	fclose(rFile);
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	sendData(socketFD, buffer); //send encrypted message

	sleep(4);

	//memset(buffer, '\0', sizeof(buffer));
	bzero(buffer, sizeof(buffer)); //for some reason memset wasn't working as intended so bzero was used
	rFile = fopen(argv[2], "r");
	if (rFile == NULL) printf("File was not opened.\n");
	fgets(buffer, sizeof(buffer), rFile);
	fclose(rFile);
	buffer[strcspn(buffer, "\n")] = '\0';
	sendData(socketFD, buffer); //send the key
	
	// Get return message from server
	//memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	bzero(buffer, sizeof(buffer));
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	buffer[strlen(buffer)-1]='\n';
		
	printf("%s", buffer); //print out decrypted message
	close(socketFD); // Close the socket

	return 0;
}
