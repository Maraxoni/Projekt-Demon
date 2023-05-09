#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>

void copyFile(const char* sourcePath, const char* destinationPath) {
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

void monitorCatalogue(const char* dir1_path, const char* dir2_path) {

	printf("Update-1");
    DIR* dir1 = opendir(dir1_path);
	DIR* dir2 = opendir(dir2_path);
    char path1[PATH_MAX];
    char path2[PATH_MAX];

    // Sprawdzenie, czy istnieje katalog 1
	printf("\nUpdate0");

    if (dir1 == NULL) {
        perror("Błąd otwierania katalogu 1");
		exit(EXIT_FAILURE);
        return;
    }
	printf("Update1");
    // Sprawdzenie, czy istnieje katalog 2
    if (dir2 == NULL) {
        perror("Błąd otwierania katalogu 2");
		exit(EXIT_FAILURE);
        return;
    }
	printf("Update2");

    // Przeiterowanie przez pliki w katalogu 1
    struct dirent* dir1_entry;
    while ((dir1_entry = readdir(dir1)) != NULL) {
        if (dir1_entry->d_type == DT_REG) {
            int found = 0;
            rewinddir(dir2);

            struct dirent* dir2_entry;
            while ((dir2_entry = readdir(dir2)) != NULL) {
                if (dir2_entry->d_type == DT_REG && strcmp(dir1_entry->d_name, dir2_entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                char src_file_path[512], dest_file_path[512];
                snprintf(src_file_path, sizeof(src_file_path), "%s/%s", dir1_path, dir1_entry->d_name);
                snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", dir2_path, dir1_entry->d_name);
                copyFile(src_file_path, dest_file_path);
            }
        }
    }

    rewinddir(dir2);

    struct dirent* dir2_entry;
    while ((dir2_entry = readdir(dir2)) != NULL) {
        if (dir2_entry->d_type == DT_REG) {
            int found = 0;
            rewinddir(dir1);

            struct dirent* dir1_entry;
            while ((dir1_entry = readdir(dir1)) != NULL) {
                if (dir1_entry->d_type == DT_REG && strcmp(dir2_entry->d_name, dir1_entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                char file_path[512];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir2_path, dir2_entry->d_name);
                deleteFile(file_path);
            }
        }
    }
    
	printf("Update3");
}

int main(int argc, char* argv[]) {
	//argv[0] = sciezka zrodlowa
	//argv[1] = sciezka docelowa
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	printf("Katalog zrodlowy: %s\n",argv[0]);
	printf("Katalog docelowy: %s\n",argv[1]);

	char path1[PATH_MAX];
	char path2[PATH_MAX];
	
	snprintf(path1, sizeof(path1),"%s" , argv[0]);
	snprintf(path2, sizeof(path2),"%s" , argv[1]);
	strcat(path1,"Data");
    printf("Nowa wartosc zmiennej path1: %s\n", path1);
    printf("Nowa wartosc zmiennej path2: %s\n", path2);
	sleep(3);
	/* Our process ID and Session ID 
	copyFile("./ProjektKatalog","./ProjektData");
	printf("Wykonano test:");*/
	int choice = 0;
	while((choice = getopt(argc,argv,""))!= -1)
	{
		switch(choice)
		{
			case 'R':
				break;
			case 't':
				break;
			case 's':
				break;
			default:
				break;
		}
	}

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

		exit(EXIT_FAILURE);
	}

	/*if ((chdir("/")) < 0) {

		exit(EXIT_FAILURE);
	}
	
	Close out the standard file descriptors 
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
    */

	/* The Big Loop */
	while (1) {
		pid = fork();
		if (pid < 0)
		{
			exit(EXIT_FAILURE);
		}

		if (pid == 0)
		{
			monitorCatalogue(path2,path1);
			exit(EXIT_SUCCESS);
		}
		sleep(7); /* wait 20 seconds */
	}
	exit(EXIT_SUCCESS);
}
