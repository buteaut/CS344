/*
 * Author: Thomas Buteau
 * Program: OTP-Keygen
 * Class: CS344-400
 * Date: 8-18-17
 * Description: The keygen program generates the cypher key for the OTP project. This program
 * will generate a string of random characters (upper case and space) equal in length to the
 * argument provided plus a null terminator.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	srand(time(NULL));	
	int num = atoi(argv[1]); //converts argv[1] to int type

	if(num <= 0) { //verify convertion was successful.
		fprintf(stderr, "keygen [number]. Number must be greater than zero.\n");
		exit(1);
	}
	//printf("The argument number is %d.\n", num); //test statement
	
	char* charBank = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	char* key = (char*) malloc((num) * sizeof(char));

	memset(key, '\0', sizeof(key));

	int i;

	for(i=0;i<num;i++) {
		key[i] = charBank[rand() % 27];
	}

	printf("%s\n",key);

	return 0;
}
