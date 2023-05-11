#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <openssl/evp.h>

#define MAX 4096
#define MAX_PATH_LENGTH 4096

// Funkcja do wyliczania sumy kontrolnej pliku przy użyciu SHA-256
void computeSHA( unsigned char* shaHash, const char* filePath) {
    int file;
    file = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    EVP_MD_CTX* shaContext = EVP_MD_CTX_new();
    const EVP_MD* shaAlgorithm = EVP_sha256();
    EVP_DigestInit_ex(shaContext, shaAlgorithm, NULL);

    unsigned char buffer[MAX];
    size_t bytesRead;

    

    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        EVP_DigestUpdate(shaContext, buffer, bytesRead);
        close(file);
        return;
    }

    EVP_DigestFinal_ex(shaContext, shaHash, NULL);

    EVP_MD_CTX_free(shaContext);
}

void copyFile(const char* sourcePath, const char* destinationPath) {
    int sourceFile, destinationFile;
    char ch;
    unsigned char buffer[4096];
    ssize_t bytesRead, bytesWritten;
    // Otwarcie pliku źródłowego w trybie tylko do odczytu
    sourceFile = open(sourcePath, O_RDONLY);

    if (sourceFile == -1) {
        perror("Błąd otwierania pliku źródłowego");
        return;
    }

    // Otwarcie pliku docelowego w trybie do zapisu
    destinationFile = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (destinationFile == -1) {
        perror("Błąd otwierania pliku docelowego");
        close(sourceFile);
        return;
    }

    while ((bytesRead = read(sourceFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = write(destinationFile, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Błąd zapisu do pliku docelowego");
            close(sourceFile);
            close(destinationFile);
            return;
        }
    }

    if (bytesRead == -1) {
        perror("Błąd odczytu z pliku źródłowego");
    }

    // Zamknięcie plików
    close(sourceFile);
    close(destinationFile);
}

void copyFileBig(const char* sourcePath, const char* destinationPath) {
    int sourceFile, destinationFile;
    char buffer[4096];
    ssize_t bytesRead, bytesWritten;
    // Otwarcie pliku źródłowego w trybie tylko do odczytu
    sourceFile = open(sourcePath, O_RDONLY);

    if (sourceFile == -1) {
        perror("Błąd otwierania pliku źródłowego");
        return;
    }

    // Otwarcie pliku docelowego w trybie do zapisu
    destinationFile = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (destinationFile == -1) {
        perror("Błąd otwierania pliku docelowego");
        close(sourceFile);
        return;
    }

    // Pobranie rozmiaru pliku źródłowego
    off_t file_size = lseek(sourceFile, 0, SEEK_END);
    if (file_size == -1) {
        perror("Błąd odczytu rozmiaru pliku źródłowego");
        close(sourceFile);
        close(destinationFile);
        return;
    }

    // Przywrócenie wskaźnika na początek pliku źródłowego
    if (lseek(sourceFile, 0, SEEK_SET) == -1) {
        perror("Błąd ustawienia wskaźnika na początek pliku źródłowego");
        close(sourceFile);
        close(destinationFile);
        return;
    }

    off_t offset = 0;
    ssize_t bytes_sent = sendfile(destinationFile, sourceFile, &offset, file_size);

    if (bytes_sent == -1) {
        perror("sendfile");
    }

    while ((bytesRead = read(sourceFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = write(destinationFile, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Błąd zapisu do pliku docelowego");
            close(sourceFile);
            close(destinationFile);
            return;
        }
    }

    if (bytesRead == -1) {
        perror("Błąd odczytu z pliku źródłowego");
    }

    // Zamknięcie plików
    close(sourceFile);
    close(destinationFile);
}

void deleteFile(const char* filePath) {
    printf("no: %s\n",filePath);
    if (unlink(filePath) == 0) {
        printf("Plik %s zostal usuniety.\n", filePath);
    }
    else {
        perror("Blad usuwania pliku");
    }
}



void deleteDirectory(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Błąd otwierania katalogu");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                deleteDirectory(path);
            } else {
                deleteFile(path);
            }
        }
    }

    closedir(dir);

    if (rmdir(dir_path) != 0) {
        perror("Błąd przy usuwaniu katalogu");
        exit(EXIT_FAILURE);
    }
}

void monitorDelete(const char* dir1_path, const char* dir2_path, bool rek) {
    DIR* dir1 = opendir(dir1_path);
    DIR* dir2 = opendir(dir2_path);

    // Sprawdzenie, czy istnieje katalog 1
    if (dir1 == NULL) {
        perror("Błąd otwierania katalogu 1");
        exit(EXIT_FAILURE);
    }
    // Sprawdzenie, czy istnieje katalog 2
    if (dir2 == NULL) {
        perror("Błąd otwierania katalogu 2");
        exit(EXIT_FAILURE);
    }

    char src_file_path[512], dest_file_path[512];
    unsigned char hash1[MAX];
    unsigned char hash2[MAX];

    // Przeiterowanie przez pliki w katalogu 2
    struct dirent* dir2_entry;
    while ((dir2_entry = readdir(dir2)) != NULL) {
        if (dir2_entry->d_type == DT_REG && strcmp(dir2_entry->d_name, ".") != 0 && strcmp(dir2_entry->d_name, "..") != 0) {
            int found = 0;
            rewinddir(dir1);

            struct dirent* dir1_entry;
            while ((dir1_entry = readdir(dir1)) != NULL) {
                if (dir1_entry->d_type == DT_REG && strcmp(dir1_entry->d_name, dir2_entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }

            if (found == 0) {
                snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", dir2_path, dir2_entry->d_name);
                deleteFile(dest_file_path);
            }
        }
    }

    rewinddir(dir2);

    // Przeiterowanie przez katalogi w katalogu 2
    while ((dir2_entry = readdir(dir2)) != NULL) {
        if (dir2_entry->d_type == DT_DIR && rek == true && strcmp(dir2_entry->d_name, ".") != 0 &&strcmp(dir2_entry->d_name, "..") != 0) {
int found = 0;
rewinddir(dir1);   struct dirent* dir1_entry;
        while ((dir1_entry = readdir(dir1)) != NULL) {
            if (dir1_entry->d_type == DT_DIR && strcmp(dir1_entry->d_name, dir2_entry->d_name) == 0) {
                found = 1;
                break;
            }
        }

        if (found == 0) {
            snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", dir2_path, dir2_entry->d_name);
            char sub_dir_path[512];
            snprintf(sub_dir_path, sizeof(sub_dir_path), "%s/%s", dir2_path, dir2_entry->d_name);
            deleteDirectory(sub_dir_path);
        } else {
            char sub_dir1_path[512], sub_dir2_path[512];
            snprintf(sub_dir1_path, sizeof(sub_dir1_path), "%s/%s", dir1_path, dir2_entry->d_name);
            snprintf(sub_dir2_path, sizeof(sub_dir2_path), "%s/%s", dir2_path, dir2_entry->d_name);
            monitorDelete(sub_dir1_path, sub_dir2_path, rek);
        }
    }
}

closedir(dir1);
closedir(dir2);

}



void monitorCatalogue(const char* dir1_path, const char* dir2_path, bool rek, int size) {

    DIR* dir1 = opendir(dir1_path);
	DIR* dir2 = opendir(dir2_path);

    // Sprawdzenie, czy istnieje katalog 1
    if (dir1 == NULL) {
        perror("Błąd otwierania katalogu 1");
		exit(EXIT_FAILURE);
    }
    // Sprawdzenie, czy istnieje katalog 2
    if (dir2 == NULL) {
        perror("Błąd otwierania katalogu 2");
		exit(EXIT_FAILURE);
    }
    char src_file_path[512], dest_file_path[512];
    unsigned char hash1[MAX];
    unsigned char hash2[MAX];
       
    // Przeiterowanie przez pliki w katalogu 1
    struct dirent* dir1_entry;
    while ((dir1_entry = readdir(dir1)) != NULL) {
        if (dir1_entry->d_type == DT_REG && strcmp(dir1_entry->d_name, ".") != 0 && strcmp(dir1_entry->d_name, "..") != 0) {
            
            int found = 0;
            rewinddir(dir2);

            

            struct dirent* dir2_entry;
            while ((dir2_entry = readdir(dir2)) != NULL) {
                snprintf(src_file_path, sizeof(src_file_path), "%s/%s", dir1_path, dir1_entry->d_name);
                snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", dir2_path, dir1_entry->d_name);
                computeSHA(hash1,src_file_path);
                computeSHA(hash2,dest_file_path);
                if (dir2_entry->d_type == DT_REG && strcmp(dir1_entry->d_name, dir2_entry->d_name) == 0) {
                    found = 1;
                    break;
                }
                /*if (dir2_entry->d_type == DT_REG && strcmp(hash1, hash2) == 0) {
                    found = 1;
                    break;
                }*/
            }

            if (found == 0) {
                copyFile(src_file_path, dest_file_path);
            }
        }

        else if (dir1_entry->d_type == DT_DIR && rek == true && strcmp(dir1_entry->d_name, ".") != 0 && strcmp(dir1_entry->d_name, "..") != 0) {
            int found = 0;
            rewinddir(dir2);

            struct dirent* dir2_entry;
            while ((dir2_entry = readdir(dir2)) != NULL) {
                if (dir2_entry->d_type == DT_DIR && strcmp(dir1_entry->d_name, dir2_entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }

            
                char src_cat_path[512], dest_cat_path[512];
                snprintf(src_cat_path, sizeof(src_cat_path), "%s/%s", dir1_path, dir1_entry->d_name);
                snprintf(dest_cat_path, sizeof(dest_cat_path), "%s/%s", dir2_path, dir1_entry->d_name);

                if (found == 0) {
                    if (mkdir(dest_cat_path, 0777) == -1) {
                        perror("Błąd przy tworzeniu katalogu");
                        exit(EXIT_FAILURE);
                    }
                }

                monitorCatalogue(src_cat_path, dest_cat_path, rek, size);
            
        }
            
            char sub_dir1_path[512], sub_dir2_path[512];
            snprintf(sub_dir1_path, sizeof(sub_dir1_path), "%s/%s", dir1_path, dir1_entry->d_name);
            snprintf(sub_dir2_path, sizeof(sub_dir2_path), "%s/%s", dir2_path, dir1_entry->d_name);
            
    }
    
    closedir(dir1);
    closedir(dir2);

}

int main(int argc, char* argv[]) {
	//argv[0] = sciezka zrodlowa
	//argv[1] = sciezka docelowa

	if (argc < 2 && argc > 5) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    if (strcmp(argv[1],"." ) == 0 || strcmp(argv[1],"./" ) == 0 ) {
		fprintf(stderr, "Can't monitor current catalogue [ %s ], your memory will have a bad time!\n", argv[1]);
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

    bool rek = false;
    int time = 300, size = 10;

	int choice = 0;
    
	while((choice = getopt(argc,argv,":t:s:R"))!= -1)
	{
		switch(choice)
		{
			case 'R':
                rek = true;
				break;
			case 't':
                time = atoi(optarg);
				break;
			case 's':
                size = atoi(optarg);
				break;
			default:
				break;
		}
	}
    printf("Nowa wartosc zmiennej time: %d\n", time);
    printf("Nowa wartosc zmiennej size: %d\n", size);

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
			monitorCatalogue(path2,path1,rek,size);
            monitorDelete(path2,path1,rek);
			exit(EXIT_SUCCESS);
		}
		sleep(time);
	}
	exit(EXIT_SUCCESS);
}
