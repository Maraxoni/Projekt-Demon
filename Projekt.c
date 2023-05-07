#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILENAME_LENGTH 512

struct FileInfo {
    char name[MAX_FILENAME_LENGTH];
    ino_t inode;
    unsigned char type;
};

void monitorCatalogue(const char* cgpath) {

    struct dirent* entry;
    struct FileInfo info;

	printf("Wywolanie funkcji\n");
	
    DIR* dir;
    dir = opendir(cgpath);

    if (dir == NULL) {
        printf("Error opening Catalogue.\n");
		exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "..") != 0) {

            char filePath[MAX_FILENAME_LENGTH];
            snprintf(filePath, sizeof(filePath), "%s/%s", cgpath, entry->d_name);

            DIR* subDir = opendir(filePath);
            if (subDir != NULL) 
			{
                printf("Found Catalogue: %s\n", entry->d_name);
                closedir(subDir);
            } 
			else 
			{
                printf("Found File: %s\n", entry->d_name);
                // Tutaj można wykonać dodatkowe operacje na znalezionych plikach
            }
        }
    }

    closedir(dir);
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
	//argv[0] = sciezka zrodlowa
	//argv[1] = sciezka docelowa
	printf("Sciezka zrodlowa:%s\n", argv[0]);
	printf("Sciezka docelowa:%s\n", argv[1]);

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Our process ID and Session ID */
	pid_t pid, sid;

	pid = fork();

	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	sid = setsid();

	if (sid < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
	if ((chdir("/")) < 0) {

		exit(EXIT_FAILURE);
	}
	

	/* Close out the standard file descriptors 
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	*/

	/* Daemon-specific initialization goes here */

	printf("Test przed petla\n");

	/* The Big Loop */
	while (1) {
		printf("Bazinga1\n");
		pid = fork();
		printf("Bazinga2\n");
		if (pid < 0)
		{
			exit(EXIT_FAILURE);
		}
		if (pid == 0)
		{
			monitorCatalogue(argv[1]);
			printf("Bazinga3 %d\n\n", pid);
		}
		sleep(7);
	}
	exit(EXIT_SUCCESS);
}
