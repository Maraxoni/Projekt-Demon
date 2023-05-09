#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILENAME_LENGTH 512

void monitorCatalogue(const char* dir1, const char* dir2) {

    DIR* dir;
    struct dirent* entry;
    struct stat statBuffer;

    // Sprawdzenie, czy istnieje katalog 1
    if ((dir = opendir(dir1)) == NULL) {
        perror("Błąd otwierania katalogu 1");
        return;
    }

    // Utworzenie katalogu 2, jeśli nie istnieje
    mkdir(dir2, 0700);

    // Przeiterowanie przez pliki w katalogu 1
    while ((entry = readdir(dir)) != NULL) {
        // Pominięcie aktualnego katalogu i katalogu nadrzędnego
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skonstruowanie pełnej ścieżki pliku w katalogu 1
        snprintf(path1, PATH_MAX, "%s/%s", dir1, entry->d_name);

        // Skonstruowanie pełnej ścieżki pliku w katalogu 2
        snprintf(path2, PATH_MAX, "%s/%s", dir2, entry->d_name);

        // Pobranie informacji o pliku
        if (stat(path1, &statBuffer) != 0) {
            perror("Błąd odczytu informacji o pliku");
            continue;
        }

        // Jeśli plik istnieje w katalogu 1, ale nie ma go w katalogu 2, skopiuj go do katalogu 2
        if (access(path2, F_OK) == -1) {
            copyFiles(path1, path2);
            continue;
        }
    }

    // Zamknięcie katalogu 1
    closedir(dir);

    // Sprawdzenie, czy istnieje katalog 2
    if ((dir = opendir(dir2)) == NULL) {
        perror("Błąd otwierania katalogu 2");
        return;
    }

    // Przeiterowanie przez pliki w katalogu 2
    while ((entry = readdir(dir)) != NULL) {
        // Pominięcie aktualnego katalogu i katalogu nadrzędnego
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skonstruowanie pełnej ścieżki pliku w katalogu 2
        snprintf(path2, PATH_MAX, "%s/%s", dir2, entry->d_name);

        // Jeśli plik istnieje w katalogu 2, ale nie ma go w katalogu 1, usuń go z katalogu 2
        if (access(path2, F_OK) != -1) {
            // Sprawdź, czy plik nie jest katalogiem
            if (stat(path2, &statBuffer) == 0 && !S_IS
DIR* dir;
    struct dirent* entry;
    char path1[PATH_MAX];
    char path2[PATH_MAX];
    struct stat statBuffer;

    // Sprawdzenie, czy istnieje katalog 1
    if ((dir = opendir(dir1)) == NULL) {
        perror("Błąd otwierania katalogu 1");
        return;
    }

    // Utworzenie katalogu 2, jeśli nie istnieje
    mkdir(dir2, 0700);

    // Przeiterowanie przez pliki w katalogu 1
    while ((entry = readdir(dir)) != NULL) {
        // Pominięcie aktualnego katalogu i katalogu nadrzędnego
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skonstruowanie pełnej ścieżki pliku w katalogu 1
        snprintf(path1, PATH_MAX, "%s/%s", dir1, entry->d_name);

        // Skonstruowanie pełnej ścieżki pliku w katalogu 2
        snprintf(path2, PATH_MAX, "%s/%s", dir2, entry->d_name);

        // Pobranie informacji o pliku
        if (stat(path1, &statBuffer) != 0) {
            perror("Błąd odczytu informacji o pliku");
            continue;
        }

        // Jeśli plik istnieje w katalogu 1, ale nie ma go w katalogu 2, skopiuj go do katalogu 2
        if (access(path2, F_OK) == -1) {
            copyFiles(path1, path2);
            continue;
        }
    }

    // Zamknięcie katalogu 1
    closedir(dir);

    // Sprawdzenie, czy istnieje katalog 2
    if ((dir = opendir(dir2)) == NULL) {
        perror("Błąd otwierania katalogu 2");
        return;
    }

    // Przeiterowanie przez pliki w katalogu 2
    while ((entry = readdir(dir)) != NULL) {
        // Pominięcie aktualnego katalogu i katalogu nadrzędnego
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skonstruowanie pełnej ścieżki pliku w katalogu 2
        snprintf(path2, PATH_MAX, "%s/%s", dir2, entry->d_name);

        // Jeśli plik istnieje w katalogu 2, ale nie ma go w katalogu 1, usuń go z katalogu 2
        if (access(path2, F_OK) != -1) {
            // Sprawdź, czy plik nie jest katalogiem
            if (stat(path2, &statBuffer) == 0 && !S_IS

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
			monitorCatalogue(argv[1],argv[0]);
			printf("Bazinga3 %d\n\n", pid);
		}
		sleep(7);
	}
	exit(EXIT_SUCCESS);
}
