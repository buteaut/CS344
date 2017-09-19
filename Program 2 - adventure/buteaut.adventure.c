/* Author: Thomas Buteau
 * Program: buteaut.adventure.c
 * Date: 7-24-17
 * Class: CS344-400
 * Description: This is the second of the two programs that comprise assignment 2.
 * 	This program will search the directories available from where it was run for
 * 	the most recently modified and read in the room files from there. It will 
 * 	use the room files' data to provide the user with a game where they will 
 * 	start in the START_ROOM and can move between the rooms via their connections
 * 	to each other until the END_ROOM is found. The path and number of steps will
 * 	be displayed at the end. Additionally, should the user enter "time" when 
 * 	prompted for a room name a text file with the current system time will be
 * 	generated and said time will also be displayed.
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct Room {
	char roomName[16];
	char connections[6][16];
	char roomType[16];
	int numConnections;
}Room;

pthread_mutex_t myMutex;

/*
 * RoomScrub writes '\0' to all strings in the Room argument and sets the 
 * numConnections to 0.
 */
void RoomScrub(struct Room *room) {
	memset(room->roomName, '\0', 16);
	memset(room->connections, '\0', 96);
	memset(room->roomType, '\0', 16);
	room->numConnections = 0;
}

/*
 * DirSearch is a function to find and return the path of the most recent ".rooms."
 * directory. Note that the code for this is taken from the reading material 2.4 from
 * canvas
 */
void DirSearch(char* curdir) {
	int newestDirTime = -1;
	char targetDirPrefix[32] = "buteaut.rooms.";
	char newestDirName[256];
	memset(newestDirName, '\0', sizeof(newestDirName));

	DIR* dirToCheck;
	struct dirent *fileInDir;
	struct stat dirAttributes;

	dirToCheck = opendir(".");

	if(dirToCheck > 0) {
		while ((fileInDir = readdir(dirToCheck)) != NULL) {
			if(strstr(fileInDir->d_name, targetDirPrefix) != NULL) {
				stat(fileInDir->d_name, &dirAttributes);

				if ((int)dirAttributes.st_mtime > newestDirTime) {
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name);
				}
			}
		}
	}
	closedir(dirToCheck);

	strcpy(curdir, newestDirName);
}

/*
 * FileNames is a function that adds the names of the files in the given
 * directory to the given array
 */
void FileNames(char* dir, char filenames[][16]) {
	int namecnt = 0;
	DIR* dirToCheck;
	struct dirent *fileInDir;
	struct stat dirAttributes;
	
	dirToCheck = opendir(dir);
	if(dirToCheck > 0) {
		while ((fileInDir = readdir(dirToCheck)) != NULL) {
			//for all the files in the directory if they are not named ".." or "." add them to the array
			if(strcmp(fileInDir->d_name, "..") != 0 && strcmp(fileInDir->d_name, ".") != 0) {
				stat(fileInDir->d_name, &dirAttributes);
				strcpy(filenames[namecnt], fileInDir->d_name);
				namecnt++;
			}
		}
	}
}

/*
 * RoomFill is a function that opens and reads in a room file and fills a room struct
 * with the data from said file
 */
void RoomFill(char* dir, char* fileName, struct Room *room) {
	//clean room
	RoomScrub(room);	
	//create filepath
	char filepath[272]; 
	memset(filepath, '\0', sizeof(filepath));
	strcpy(filepath, dir);
	strcat(filepath, "/");
	strcat(filepath, fileName);

	//create buffer string
	char buffer[32];
	memset(buffer, '\0', sizeof(buffer));
	
	int concount = 0; //keeps track of connections so far in file
	//open file
	FILE *rFile;
	rFile = fopen(filepath, "r");
	if (rFile == NULL) printf("File was not opened.\n");
	//fill buffer
	do {
		memset(buffer, '\0', sizeof(buffer));
		fgets(buffer, 32, rFile);
		size_t size = strlen(buffer)-1;
		if(buffer[size] == '\n') buffer[size] = '\0';//removes the newline character at the end of buffer
		
		if(strstr(buffer, "ROOM NAME") != NULL) {
			//copy from buffer[12] to end of buffer into room.roomName
			char* modbuffer = buffer+11;
			strcpy(room->roomName, modbuffer);
		}

		else if(strstr(buffer, "CONNECTION") != NULL) {
			//copy from buffer[15] to end of buffer into room.connections[concount]
			char* modbuffer = buffer+14;
			strcpy(room->connections[concount], modbuffer);
			concount++;
			room->numConnections++;
		}

		else if(strstr(buffer, "ROOM TYPE") != NULL) {
			//copy from buffer[12] to the end of buffer into room.roomType
			char* modbuffer = buffer+11;
			strcpy(room->roomType, modbuffer);
		}

	} while (strstr(buffer, "ROOM TYPE") == NULL);
}

/*
 * Time is a function that gets the current time and writes it to a file.
 */
void* Time(void* var) {
	char* filepath = "currentTime.txt";
	time_t currentTime;
	char timeBuffer[80];
	memset(timeBuffer, '\0', sizeof(timeBuffer));
	struct tm * timeinfo; //use of timeinfo taken from example shown at www.cplusplus.com/reference/ctime/strftime/?kw=strftime
	
	time(&currentTime);
	timeinfo = localtime(&currentTime);

	strftime(timeBuffer, 80, "%l:%M%P, %A, %B %e, %Y", timeinfo);

	FILE *tFile;
	pthread_mutex_lock(&myMutex);
	tFile = fopen(filepath, "w");
	if (tFile == NULL) printf("File was not opened.\n");
	fprintf(tFile, timeBuffer);
	fclose(tFile);
	pthread_mutex_unlock(&myMutex);
	return(NULL);
}


/*
 * Game prompts the user with the current room information and has the user choose the next room in a loop
 * until the room with the END_ROOM type is reached. It also keeps track of the rooms visited and how many
 * steps were needed to reach the END_ROOM. 
 */
void Game(struct Room room, char* dir) {
	int steps = 0;
	char path[100][16];
	memset(path, '\0', sizeof(path));
	char *choice;
	size_t choicesize = 16;
	int i;

	while(strcmp(room.roomType, "END_ROOM") != 0) {
		//code block prints out the current location and room choices in the required format
		printf("CURRENT LOCATION: %s\n", room.roomName);
		printf("POSSIBLE CONNECTIONS: ");
		for(i=0;i<room.numConnections-1;i++) {
			printf("%s, ", room.connections[i]);
		}	
		printf("%s.\n", room.connections[room.numConnections-1]);
		printf("WHERE TO? >");	
		//code block for accepting user input and removing newline character
		choice = (char*) malloc(choicesize);
		getline(&choice, &choicesize , stdin);
		size_t size = strlen(choice)-1;
		if(choice[size] == '\n') choice[size] = '\0';
		printf("\n\n");

		//code block to handle "time" input
		if(strcmp(choice, "time") == 0) {
			//Time(NULL); //will be modified when mutex is used
			pthread_t chronothread;
			pthread_create(&chronothread, NULL, Time, NULL);
			pthread_join(chronothread, NULL);
			char *buffer;
			size_t buffersize = 64;
			buffer = (char*) malloc(buffersize);
			FILE *tFile;
			tFile = fopen("currentTime.txt", "r");
			getline(&buffer, &buffersize, tFile);
			printf("%s\n\nWHERE TO? >", buffer);
			free(buffer);
			free(choice);
			choice = (char*) malloc(choicesize);
			getline(&choice, &choicesize, stdin);
			size_t size = strlen(choice)-1;
			if(choice[size] == '\n') choice[size] = '\0';
		}		


		//code block to check user input against connected rooms
		int valid = 0; //flag to see if user input is valid
		for(i=0;i<room.numConnections;i++) {
			if(strcmp(choice, room.connections[i]) == 0) {
				valid = 1;
				strcpy(path[steps], room.roomName);
				steps++;
				RoomFill(dir, choice, &room);
			}
		}
		//input is not a valid room
		if(valid == 0) printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n\n");
		free(choice);
	}

	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
	for(i=0;i<steps;i++) {
		printf("%s\n", path[i]);
	}
}

int main() {
	char curdir[256]; //stores directory of room files
	memset(curdir, '\0', sizeof(curdir));
	char filename[16]; //stores name of room file to be used
	memset(filename, '\0', sizeof(filename));
	struct Room curroom;
	RoomScrub(&curroom);
	
	char filelist[7][16]; //list of rooms in the directory
	memset(filelist, '\0', sizeof(filelist));

	DirSearch(curdir);
	FileNames(curdir, filelist);
	
	//This for loop cycles through all the room files until the "START_ROOM" file is found.
	int i;
	for(i=0;i<7;i++){
		RoomFill(curdir, filelist[i], &curroom);
		if(strcmp(curroom.roomType, "START_ROOM") == 0) break;
	}

	Game(curroom, curdir);
	pthread_mutex_destroy(&myMutex);
	return 0;
}
