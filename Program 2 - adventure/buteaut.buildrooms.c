/* Author: Thomas Buteau
 * Program: buteaut.buildrooms
 * Date: 7-24-17
 * Class: CS344-400
 * Description: This is the first of two programs for the "Program 2" assignment
 * 	The purpose of this program is to create a new directory and fill it with
 * 	room files. The room files consist of the room name, 3 to 6 connections
 * 	to other rooms, and the room type. This program will randomly pick 7 of
 * 	10 possible room names to use and randomly generate connections between
 * 	them until all rooms have 3 to 6 connections. There will be three room
 * 	types, START_ROOM, MID_ROOM, and END_ROOM. START_ROOM and END_ROOM will
 * 	both be single rooms while the other five will be MID_ROOMs.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct Room{
	char roomName[16];
	char connections[6][16];
	char roomType[16];
	int numConnections;
} Room;

/*
 * stringCleaning takes in a struct Room and scrubs all strings to be '\0'
 */ 
void StringCleaning(struct Room room) {
	memset(room.roomName, '\0', 16);
	memset(room.connections, '\0', 96);
	memset(room.roomType, '\0', 16);
}

/*
 * generateRoomList take an empty array of ints and fills the first
 * 7 positions with random, non-repeated numbers in the range of 
 * 0 to 9.
 */
void GenerateRoomList(int* list) {
	int filled = 0;
	while(filled < 7) {
		int cur = rand() % 10;
		int repeat = 0;
		int loop;
		for (loop=0;loop<filled;loop++) {
			if(list[loop] == cur) repeat = 1;
		} 
		if (repeat == 0) {
			list[filled] = cur;
			filled++;
		}
	}
}

/*
 * roomInit fills a struct Room array with rooms that are initialized
 * with room names based on the 2d char array options using the int
 * array list to determine which names from options to use. 
 * The first room initialized will receive the "START_ROOM" type, the
 * last room initialized will receive the "END_ROOM" type, all others
 * will be the "MID_ROOM" type. 
 */
void RoomInit(struct Room* rooms, int* list, char options[][16]) {
	int i;
	for(i=0;i<7;i++) {
		strcpy(rooms[i].roomName, options[list[i]]); 
		if (i==0) {
			 strcpy(rooms[i].roomType, "START_ROOM");
		}
		else if (i==6) {
			strcpy(rooms[i].roomType, "END_ROOM");
		}
		else {
			strcpy(rooms[i].roomType, "MID_ROOM");
		}
		rooms[i].numConnections=0;
	}
}

/*
 * finishedConnecting is a function suggested in Canvas document "2.2 Program
 * Outlining in Program 2" which checks if the 3 connection per room condition
 * has been met and returns a bool to that effect.
 */
int FinishedConnecting(struct Room *rooms) {
	int full = 1;
	int i;
	for(i=0;i<7;i++){
		if(rooms[i].numConnections < 3) {
			full = 0; 
		}
	}
	return full;
}

/*
 * GetRandomRoom returns a random number to be used to pick a room in the room
 * array.
 */
int GetRandomRoom() {
	int i = rand() % 7;
	return i;
}

/*
 * CanAddConnectionFrom checks the number of connections, if
 * there are less than 6 returns 1 else return 0
 */
int CanAddConnectionFrom(struct Room room) {
	int answer = 0;
	if (room.numConnections < 6) answer = 1;
	return answer;
}

/*
 * ConnectRoom adds y to x's connections, x to y's connections
 * and increments both x and y's numConnections
 */
void ConnectRoom(struct Room *rooms, int x, int y) {
	strcpy(rooms[x].connections[rooms[x].numConnections], rooms[y].roomName);
	rooms[x].numConnections++;
	strcpy(rooms[y].connections[rooms[y].numConnections], rooms[x].roomName);
	rooms[y].numConnections++;
}

/*
 * IsSameRoom compares the roomName of the two Room arguments and
 * returns 1 if they match and 0 if they do not.
 */
int IsSameRoom(Room x, Room y) {
	int same = 0;
	if(strcmp(x.roomName, y.roomName) == 0) same = 1;
	return same;
}

/*
 * IsConnected checks if there is already a connection between
 * the two room arguments
 */
int IsConnected(Room x, Room y) {
	int connected = 0;
	int i;
	for(i=0;i<x.numConnections;i++) {
		if(strcmp(x.connections[i],y.roomName) == 0) connected = 1;
	}
	return connected;
}

/*
 * AddRandomConnection finds two rooms that can add more connections which
 * are not connected to each other already and connects them.
 */
void AddRandomConnection(struct Room* gameRooms) {
	int A;
	int B;

	int loop = 1;
	while(loop == 1) {
		A = GetRandomRoom(gameRooms);
		if(CanAddConnectionFrom(gameRooms[A]) == 1) loop = 0;
	}
	do {
		B = GetRandomRoom(gameRooms);
	} while(CanAddConnectionFrom(gameRooms[B]) == 0 || IsSameRoom(gameRooms[A], gameRooms[B]) == 1 || IsConnected(gameRooms[A], gameRooms[B]) == 1);
	ConnectRoom(gameRooms, A, B);
}

/*
 * RoomString generates a string of the room data in the format required to export to files.
 * Note that char string array argument must be able to fit 256 characters.
 */
void RoomString(char* string, struct Room room) {
	char formatted[256];
	memset(formatted, '\0', 256);	
	char buffer[31];
	memset(buffer, '\0', 31);
	int n;

	//add room name
	n=sprintf(buffer, "ROOM NAME: %s\n", room.roomName);
	strcat(formatted, buffer);

	//add connections
	int i;
	for(i=0;i<room.numConnections;i++) {
		memset(buffer, '\0', 31);
		n=sprintf(buffer, "CONNECTION %d: %s\n", i+1, room.connections[i]);
		strcat(formatted, buffer);
	}

	//add room type
	memset(buffer, '\0', 31);
	n=sprintf(buffer, "ROOM TYPE: %s", room.roomType);
	strcat(formatted, buffer);
	
	strcpy(string, formatted);
}


/*
 * WriteToFile takes in an array of room objects as well as the directory to store files in
 * and write the room files
 */
void WriteToFile(struct Room* gameRooms, char* directory) {
	char dircpy[36], filecpy[256];
	int i, n;
	FILE * rFile;
	for(i=0;i<7;i++) {
		//clear dircpy then regenerate it for the current file to write
		memset(dircpy, '\0', 36);
		n=sprintf(dircpy, "%s/%s", directory, gameRooms[i].roomName);
		
		//clear filecpy then fill it with the contents for current file
		memset(filecpy, '\0', 256);
		RoomString(filecpy, gameRooms[i]);
		
		//open file, write data, close file
		rFile = fopen(dircpy, "w");
		if (rFile !=NULL) {
			fputs(filecpy, rFile);
			fclose(rFile);
		} 
	}
}

int main() {
	srand(time(NULL));
	//This section determines the process id and creates the directory for the files
	//code for process id largely taken from piazza post @42 with minor correction
	//to pid_len
	int pid = getpid();
	int pid_len = snprintf(NULL, 0, "%d", pid);
	char* dirstr = (char*) malloc( pid_len * sizeof(char) + 1 );
	sprintf(dirstr, "buteaut.rooms.%d", pid);

	int directory = mkdir(dirstr, 0755);

	/* code block to test struct Room functions correctly
	Room test;
	StringCleaning(test);
	strcpy(test.roomName, "test");
	strcpy(test.roomType, "START_ROOM");
	test.numConnections = 0;
	printf("The room name is %s, the type is %s, and the number of connections is %d.\n", 
		test.roomName, test.roomType, test.numConnections);
	*/

	//My hat is off to you dear grader if you get the reference with the room names chosen
	char roomOptions[10][16] = {"Kingdom End\0\0\0\0\0", "Profit Share\0\0\0\0", 
		"Argon Prime\0\0\0\0\0", "Omicron Lyrae\0\0\0", "Legend's Home\0\0\0", 
		"Montalaar\0\0\0\0\0\0\0", "Family Rhy\0\0\0\0\0\0", "Seizewell\0\0\0\0\0\0\0", 
		"Heretic's End\0\0\0", "Paranid Prime\0\0\0"};
	
	/* code block to test roomOptions is correct	
	int i = 0;
	for(i=0; i<10; i++) {
		printf("roomOption %d is %s.\n", i, roomOptions[i]);
	}
	*/

	int roomList[10];	
	GenerateRoomList(roomList);

	/* code block to test generateRoomList
	int i;
	for (i=0;i<7;i++){
		printf("roomList position %d is %d.\n", i, roomList[i]);
	}
	*/
	
	struct Room gameRooms[7];	
	RoomInit(gameRooms, roomList, roomOptions);
	
	/* code block to test roomInit 
	int i;
	for(i=0;i<7;i++) {
		printf("gameRooms position %d has the name %s and is type %s.\n", i, gameRooms[i].roomName, gameRooms[i].roomType);
	}
	*/

	// code block to test FinishedConnecting
	//if(FinishedConnecting(gameRooms)== 0) printf("FinishedConnecting working as intended.\n");

	// code block to test CanAddConnectionFrom
	//if(CanAddConnectionFrom(gameRooms[0]) == 1) printf("CanAddConnectionFrom working as intended.\n");
	//printf("Finished RoomInit function, starting AddRandomConnection loop.\n");

	while(FinishedConnecting(gameRooms) == 0) {
		AddRandomConnection(gameRooms);
	}

	/*code block to test AddRandomConnection and RoomString
	char string[256];
	int i;
	for(i=0;i<7;i++) {
		RoomString(string, gameRooms[i]);
		printf("%s\n", string);
	}
	*/
	
	WriteToFile(gameRooms, dirstr);
	free(dirstr);
	return 0;
}
