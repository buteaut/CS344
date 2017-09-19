/*
 * Author: Thomas Buteau
 * Program: OTP - otp_enc
 * Class: CS344-400
 * Date: 8-18-17
 * Description: The otp_enc program connects to the otp_enc_d program via the port provided as an argument and
 * 		sends otp_enc_d the message as well as the key and then receives the encrypted message that it
 * 		then prints to stdout. Note that this program will verify that the key is not shorter than the
 * 		message before sending to otp_enc_d. 
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
	//printf("Client sending data: %s\n", message);
	//printf("Client: data sent has length of %d\n", strlen(message));
	charsWritten = send(socketFD, message, strlen(message), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(message)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	//printf("Client finished sending data\n");
}
 
int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[70000];

	if (argc < 3) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args
	
	//this block of code checks if the key is shorter than the message and exits if true
	char input1[70000];
	char input2[70000];
	memset(input1, '\0', sizeof(input1));
	memset(input2, '\0', sizeof(input2));
	FILE *tFile;
	tFile = fopen(argv[1], "r");
	fgets(input1, sizeof(input1), tFile);
	fclose(tFile);
	input1[strcspn(input1, "\n")] = '\0';
	tFile = fopen(argv[2], "r");
	fgets(input2, sizeof(input2), tFile);
	fclose(tFile);
	input2[strcspn(input2, "\n")] = '\0';
	if(strlen(input1) > strlen(input2)) {
		fprintf(stderr, "key is too short");
		exit(1);
	}

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
	rFile = fopen(argv[1], "r"); //opens the message argument
	if (rFile == NULL) printf("File was not opened.\n");
	fgets(buffer, sizeof(buffer), rFile); //writes message to buffer
	fclose(rFile);
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	sendData(socketFD, buffer);

	sleep(4); //sleep used to allow otp_enc_d to receive message and get ready to receive key before key starts to send

	//memset(buffer, '\0', sizeof(buffer));
	bzero(buffer, sizeof(buffer)); //for some reason memset was behaving oddly so bzero was used instead
	
	rFile = fopen(argv[2], "r"); //opens the key argument
	if (rFile == NULL) printf("File was not opened.\n");
	fgets(buffer, sizeof(buffer), rFile); //writes ket to buffer
	fclose(rFile);
	buffer[strcspn(buffer, "\n")] = '\0';
	sendData(socketFD, buffer);
	
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	printf("%s",buffer); //prints out the encrypted message

	sleep(4);
	
	close(socketFD); // Close the socket

	return 0;
}
