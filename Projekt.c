#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

void copyFiles(const char* sourcePath, const char* destinationPath) {
    FILE* sourceFile, * destinationFile;
    char ch;

    // Otwarcie pliku źródłowego w trybie tylko do odczytu
    sourceFile = fopen(sourcePath, "r");
    if (sourceFile == NULL) {
        perror("Błąd otwierania pliku źródłowego");
        return;
    }

    // Otwarcie pliku docelowego w trybie do zapisu
    destinationFile = fopen(destinationPath, "w");
    if (destinationFile == NULL) {
        perror("Błąd otwierania pliku docelowego");
        fclose(sourceFile);
        return;
    }

    // Kopiowanie zawartości pliku ze źródła do celu
    while ((ch = fgetc(sourceFile)) != EOF) {
        fputc(ch, destinationFile);
    }

    // Zamknięcie plików
    fclose(sourceFile);
    fclose(destinationFile);
}

void deleteFile(const char* filePath) {
    if (unlink(filePath) == 0) {
        printf("Plik %s został usunięty.\n", filePath);
    }
    else {
        perror("Błąd usuwania pliku");
    }
}

void compareDirectories(const char* dir1, const char* dir2) {
    DIR* dir;
    struct dirent* entry;
    char path1[PATH_MAX];
    char path2[PATH_MAX];
    int fileExistsInDir1, fileExistsInDir2;

    // Sprawdzenie, czy istnieje katalog 1
    if ((dir = opendir(dir1)) == NULL) {
        perror("Błąd otwierania katalogu 1");
        return;
    }

    // Sprawdzenie, czy istnieje katalog 2
    if ((dir = opendir(dir2)) == NULL) {
        perror("Błąd otwierania katalogu 2");
        return;
    }

    // Przeiterowanie przez pliki w katalogu 1
    while ((entry = readdir(dir)) != NULL) {
        // Pominięcie aktualnego katalogu i katalogu nadrzędnego
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skonstruowanie pełnej ścieżki pliku w katalogu 1
        snprintf(path1, PATH_MAX, "%s/%s", dir1, entry->d_name);

        // Skonstruowanie pełnej ścieżki pliku w katalogu 2
        snprintf(path2, PATH_MAX, "%s/%s", dir2, entry->d_name);

        // Sprawdzenie, czy plik istnieje w katalogu 2
        fileExistsInDir2 = access(path2, F_OK) != -1;

        // Jeśli plik istnieje w katalogu 1, ale nie ma go w katalogu 2, skopiuj go do katalogu
        if (!fileExistsInDir2) {
            copyFiles(path1, path2);
            continue;
        }

        // Jeśli plik istnieje w katalogu 2, ale nie ma go w katalogu 1, usuń go z katalogu 2
        fileExistsInDir1 = access(path1, F_OK) != -1;
        if (!fileExistsInDir1) {
            deleteFile(path2);
        }
    }

    // Zamknięcie katalogu
    closedir(dir);
}

int main(int argc, char* argv[]) {
	//argv[0] = sciezka zrodlowa
	//argv[1] = sciezka docelowa
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then
	   we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);

	/* Open any logs here */

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
	if ((chdir("/")) < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Daemon-specific initialization goes here */

	/* The Big Loop */
	while (1) {
		/* Do some task here ... */
		pid = fork();
		if (pid < 0)
		{
			exit(EXIT_FAILURE);
		}

		if (pid == 0)
		{
			monitorCatalogue(argv[0]);
		}
		sleep(20); /* wait 20 seconds */
	}
	exit(EXIT_SUCCESS);
}

