#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <openssl/evp.h>
#include <sys/stat.h>

#define USER_TRIGGER SIGUSR1
#define MAX_PATH_LENGTH 4096
int ifUserInput = 0;

//Sygnal
void handleSignal(int signal)
{
	if(signal== USER_TRIGGER){
		fprintf(stdout,"Obudzenie przez uzytkownika");
		syslog(LOG_INFO,"Sygnal uzytkowinka. Wybudzenie demona");
		ifUserInput = 1;
	}	
	else{
		syslog(LOG_INFO,"%d\n", signal);
	}
}

//Zbieranie procesow child, aby nie tworzyly sie procesy "zombie" czyli <defunct>
void sigchld(int fd) {
  waitpid(-1,NULL,WNOHANG);
}

// Funkcja do wyliczania sumy kontrolnej pliku
void computeSHA(unsigned char* shaHash, const char* filePath) {
    // Otwieranie pliku w trybie tylko do odczytu
    int file = open(filePath, O_RDONLY);
    if (file == -1) {
        // Obsluga bledu otwarcia pliku
        syslog(LOG_ERR, "Blad otwierania pliku");
        return;
    }
    // Wykorzystanie algorytmu ssl
    EVP_MD_CTX* shaContext = EVP_MD_CTX_new();
    const EVP_MD* shaAlgorithm = EVP_sha256();
    EVP_DigestInit_ex(shaContext, shaAlgorithm, NULL);
    // Bufor
    unsigned char buffer[MAX_PATH_LENGTH];
    ssize_t bytesRead;
    // Odczytywanie sumy kontrolnej
    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        EVP_DigestUpdate(shaContext, buffer, bytesRead);
    }
    // Zamykanie pliku
    close(file);
    // Suma kontrolna
    EVP_DigestFinal_ex(shaContext, shaHash, NULL);
    EVP_MD_CTX_free(shaContext);
}

long getFileSize(const char* filePath) {
    // Otwieranie pliku w trybie tylko do odczytu
    int file = open(filePath, O_RDONLY);
    if (file == -1) {
        perror("Blad otwierania pliku");
        syslog(LOG_ERR, "Blad otwierania pliku");
        return -1;
    }

    // Przesuwanie wskaznika na koniec
    off_t offset = lseek(file, 0, SEEK_END);
    if (offset == -1) {
        perror("Blad ustawienia wskaznika");
        syslog(LOG_ERR, "Blad ustawienia wskaznika");
        close(file);
        return -1;
    }

    // Pobieranie pozycji wskaznika rownej wielkosci pliku
    long size = (long)offset;
    // Zamykanie pliku i zwracanie wartosci wielkosci
    close(file);
    return size;
}

void copyFile(const char* srcPath, const char* dstPath) {
    int srcFile, dstFile;
    char buffer[4096];
    ssize_t bytesRead, bytesWritten;

    // Otwieranie pliku zrodlowego w trybie tylko do odczytu
    srcFile = open(srcPath, O_RDONLY);
    if (srcFile == -1) {
        perror("Blad przy otwieraniu pliku zrodlowego");
        syslog(LOG_ERR, "Blad przy otwieraniu pliku docelowego");
        return;
    }

    // Otwieranie pliku docelowego w trybie tylko do zapisu i tworzenie go jezeli go nie ma
    dstFile = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dstFile == -1) {
        perror("Blad przy otwieraniu pliku docelowego");
        syslog(LOG_ERR, "Blad przy otwieraniu pliku docelowego");
        close(srcFile);
        return;
    }

    // Kopiowanie zawartosci pliku zrodlowego do docelowego
    while ((bytesRead = read(srcFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = write(dstFile, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Blad przy zapisie do pliku docelowego");
            syslog(LOG_ERR, "Blad przy otwieraniu pliku docelowego");
            break;
        }
    }

    // Sprawdzanie czy wystapil blad podczas odczytu
    if (bytesRead == -1) {
        perror("Blad przy odczycie pliku zrodlowego");
        syslog(LOG_ERR, "Blad przy otwieraniu pliku zrodlowego");
    }

    // Zamkanie plikow
    close(srcFile);
    close(dstFile);
}


void copyFileBig(const char* srcPath, const char* dstPath) {
    int srcFile, dstFile;
    off_t offset = 0;

    // Otwieranie pliku zrodlowego w trybie do odczytu
    srcFile = open(srcPath, O_RDONLY);
    if (srcFile == -1) {
        perror("Blad przy otwieraniu pliku zrodlowego");
        syslog(LOG_ERR, "Blad przy otwieraniu pliku zrodlowego");
        return;
    }

    // Otwieranie pliku docelowego w trybie tylko do zapisu i tworzenie go jezeli go nie ma
    dstFile = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dstFile == -1) {
        perror("Blad przy otwieraniu pliku docelowego");
        syslog(LOG_ERR, "Blad przy otwieraniu pliku docelowego");
        close(srcFile);
        return;
    }

    // Kopiowanie zawartosci pliku zrodlowego do docelowego
    ssize_t bytesSent;
    while ((bytesSent = sendfile(dstFile, srcFile, &offset, 4096)) > 0) {
        // Sprawdzanie czy wystapil blad podczas kopiowania
        if (bytesSent == -1) {
            perror("Blad przy kopiowaniu pliku");
            syslog(LOG_ERR, "Blad przy kopiowaniu pliku");
            break;
        }
    }

    // Zamykanie plikow
    close(srcFile);
    close(dstFile);
}


void deleteFile(const char* filePath) {
    //Usuwanie pliku i sprawdzanie czy sie usunal
    if (unlink(filePath) == 0) {
        fprintf(stdout,"Plik %s zostal usuniety.\n", filePath);
    }
    else {
        perror("Blad usuwania pliku");
        syslog(LOG_ERR, "Blad usuwania pliku");
    }
}



void deleteDirectory(const char* dirPath) {
    //Otworzenie katalogu i sprawdzenie czy sie otworzyl
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Blad otwierania katalogu");
        syslog(LOG_ERR, "Blad otwierania katalogu");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    //Rekurencyjne usuwanie zawartosci katalogu
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name);

            if (entry->d_type == DT_DIR) {
                deleteDirectory(path);
            } else {
                deleteFile(path);
            }
        }
    }
    closedir(dir);
    //Usuwanie katalogu i sprawdzanie czy sie usunal
    if (rmdir(dirPath) != 0) {
        perror("Blad przy usuwaniu katalogu");
        syslog(LOG_ERR, "Blad przy usuwaniu katalogu");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(stdout,"Katalog %s zostal usuniety.\n", dirPath);
    }
}

void monitorDelete(const char* srcPath, const char* dstPath, bool rek) {
    // Otworzenie katalogow
    DIR* dir1 = opendir(srcPath);
    DIR* dir2 = opendir(dstPath);

    // Sprawdzenie, czy istnieje katalog 1
    if (dir1 == NULL) {
        perror("Blad otwierania katalogu 1");
        syslog(LOG_ERR, "Blad otwierania katalogu 1");
        exit(EXIT_FAILURE);
    }
    // Sprawdzenie, czy istnieje katalog 2
    if (dir2 == NULL) {
        perror("Blad otwierania katalogu 2");
        syslog(LOG_ERR, "Blad otwierania katalogu 2");
        exit(EXIT_FAILURE);
    }

    char srcFilePath[512], dstFilePath[512];
    unsigned char hash1[MAX_PATH_LENGTH];
    unsigned char hash2[MAX_PATH_LENGTH];

    // Przeiterowanie przez pliki w katalogu 2
    struct dirent* dir2Entry;
    while ((dir2Entry = readdir(dir2)) != NULL) {
        if (dir2Entry->d_type == DT_REG && strcmp(dir2Entry->d_name, ".") != 0 && strcmp(dir2Entry->d_name, "..") != 0) {
            int found = 0;
            rewinddir(dir1);
            // Przeiterowanie przez pliki w katalogu 1
            struct dirent* dir1Entry;
            while ((dir1Entry = readdir(dir1)) != NULL) {
                if (dir1Entry->d_type == DT_REG && strcmp(dir1Entry->d_name, dir2Entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }
            //Jezeli nie znajdzie pliku w katalogu zrodlowym to usuwa plik
            if (found == 0) {
                snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, dir2Entry->d_name);
                deleteFile(dstFilePath);
                syslog(LOG_INFO, "Usunieto plik: %s", dstFilePath);
            }
        }
    }

    rewinddir(dir2);

    // Przeiterowanie przez katalogi w katalogu 2
    while ((dir2Entry = readdir(dir2)) != NULL) {
        if (dir2Entry->d_type == DT_DIR && rek == true && strcmp(dir2Entry->d_name, ".") != 0 &&strcmp(dir2Entry->d_name, "..") != 0) {
            int found = 0;
            rewinddir(dir1);   
            struct dirent* dir1Entry;
            // Przeiterowanie przez katalogi w katalogu 1
            while ((dir1Entry = readdir(dir1)) != NULL) {
                if (dir1Entry->d_type == DT_DIR && strcmp(dir1Entry->d_name, dir2Entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }
            //Jezeli nie znajdzie katalogu w katalogu zrodlowym, usuwa katalog
            //W przeciwnym razie wchodzi rekurencyjnie do tego katalogu
            if (found == 0) {
                snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, dir2Entry->d_name);
                char sub_dirPath[512];
                snprintf(sub_dirPath, sizeof(sub_dirPath), "%s/%s", dstPath, dir2Entry->d_name);
                deleteDirectory(sub_dirPath);
                syslog(LOG_INFO, "Usunieto katalog: %s", sub_dirPath);
            } else {
                char sub_srcPath[512], sub_dir2Path[512];
                snprintf(sub_srcPath, sizeof(sub_srcPath), "%s/%s", srcPath, dir2Entry->d_name);
                snprintf(sub_dir2Path, sizeof(sub_dir2Path), "%s/%s", dstPath, dir2Entry->d_name);
                monitorDelete(sub_srcPath, sub_dir2Path, rek);
            }
        }
    }

closedir(dir1);
closedir(dir2);
}

void monitorCatalogue(const char* srcPath, const char* dstPath, bool rek, long size) {
    // Otworzenie katalogow
    DIR* dir1 = opendir(srcPath);
	DIR* dir2 = opendir(dstPath);

    // Sprawdzenie, czy istnieje katalog 1
    if (dir1 == NULL) {
        perror("Blad otwierania katalogu 1");
		exit(EXIT_FAILURE);
    }
    // Sprawdzenie, czy istnieje katalog 2
    if (dir2 == NULL) {
        perror("Blad otwierania katalogu 2");
        syslog(LOG_ERR, "Skopiowano plik: %s", dstPath);
		exit(EXIT_FAILURE);
    }
    // Deklaracja sciezek
    char srcFilePath[512], dstFilePath[512];
    unsigned char hash1[MAX_PATH_LENGTH];
    unsigned char hash2[MAX_PATH_LENGTH];
       
    // Przeiterowanie przez pliki w katalogu 1
    struct dirent* dir1Entry;
    while ((dir1Entry = readdir(dir1)) != NULL) {
        if (dir1Entry->d_type == DT_REG && strcmp(dir1Entry->d_name, ".") != 0 && strcmp(dir1Entry->d_name, "..") != 0) {
            // Przeiterowanie przez pliki w katalogu 2, w celu znalezienia brakujacych plikow w katalogu docelowym
            int found = 0;
            rewinddir(dir2);
            struct dirent* dir2Entry;
            while ((dir2Entry = readdir(dir2)) != NULL) {
                snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, dir1Entry->d_name);
                snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, dir1Entry->d_name);
                computeSHA(hash1,srcFilePath);
                computeSHA(hash2,dstFilePath);
                if (dir2Entry->d_type == DT_REG && strcmp(hash1, hash2) == 0) {
                    found = 1;
                    break;
                }
            }
            // Jezeli nie znaleziono zadnych pasujacych plikow to wykonywane jest kopiowanie
            if (found == 0) {
                //Porownywanie wielkosci pliku z size
                if(getFileSize(srcFilePath)<size)
                {
                    copyFile(srcFilePath, dstFilePath);
                }
                else
                {
                    copyFileBig(srcFilePath, dstFilePath);
                }
                syslog(LOG_INFO, "Skopiowano plik: %s", dstFilePath);
            }
        }
        else if (dir1Entry->d_type == DT_DIR && rek == true && strcmp(dir1Entry->d_name, ".") != 0 && strcmp(dir1Entry->d_name, "..") != 0) {
            int found = 0;
            rewinddir(dir2);
            struct dirent* dir2Entry;

            while ((dir2Entry = readdir(dir2)) != NULL) {
                if (dir2Entry->d_type == DT_DIR && strcmp(dir1Entry->d_name, dir2Entry->d_name) == 0) {
                    found = 1;
                    break;
                }
            }
            //Deklaracja sciezek
            char srcCatPath[512], dstCatPath[512];
            snprintf(srcCatPath, sizeof(srcCatPath), "%s/%s", srcPath, dir1Entry->d_name);
            snprintf(dstCatPath, sizeof(dstCatPath), "%s/%s", dstPath, dir1Entry->d_name);
            //Jezeli nie znajdzie katalogu to go tworzy
            if (found == 0) {
                if (mkdir(dstCatPath, 0777) == -1) {
                    perror("Blad przy tworzeniu katalogu");
                    syslog(LOG_ERR, "Blad przy tworzeniu katalogu");
                    exit(EXIT_FAILURE);
                }
            }
            //Wywolanie rekurencyjne wchodzace do katalogow
            monitorCatalogue(srcCatPath, dstCatPath, rek, size);
        } 
    }
    
    closedir(dir1);
    closedir(dir2);
}

int main(int argc, char* argv[]) {
	//argv[0] = sciezka zrodlowa
	//argv[1] = sciezka docelowa
	
	if (argc < 2 && argc > 5) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        syslog(LOG_ERR, "Zle argumenty");
		exit(EXIT_FAILURE);
	}

    if (strcmp(argv[1],"." ) == 0 || strcmp(argv[1],"./" ) == 0 ) {
		fprintf(stderr, "Can't monitor current catalogue [ %s ], your memory will have a bad time!\n", argv[1]);
        syslog(LOG_ERR, "Zly katalog");
		exit(EXIT_FAILURE);
	}
	signal(SIGUSR1,handleSignal);
	    signal(SIGCHLD,sigchld);
	openlog("signalProgram", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	openlog("DEMON", LOG_PID|LOG_CONS, LOG_USER);
	syslog(LOG_INFO, "Inicjalizacja");
	fprintf(stdout,"Inicjalizacja\n");
    //Zmiana argumentow na sciezki
	fprintf(stdout,"Katalog zrodlowy: %s\n",argv[0]);
	fprintf(stdout,"Katalog docelowy: %s\n",argv[1]);
	char path1[PATH_MAX];
	char path2[PATH_MAX];
	snprintf(path1, sizeof(path1),"%s" , argv[0]);
	snprintf(path2, sizeof(path2),"%s" , argv[1]);
	strcat(path1,"Data");
    fprintf(stdout,"Nowa wartosc zmiennej path1: %s\n", path1);
    fprintf(stdout,"Nowa wartosc zmiennej path2: %s\n", path2);
    //Deklaracja zmiennych
    bool rek = false;
    long size = 10;
    int time = 300, choice = 0;
    
    //Wczytanie argumentow przez getopta
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
    
    //Kontrola
    fprintf(stdout,"Nowa wartosc zmiennej time: %d\n", time);
    fprintf(stdout,"Nowa wartosc zmiennej size: %ld\n", size);
    //Zmiana na mB
    size = size * 1024 * 1024;

    //Forkowanie
	pid_t pid, sid;
	pid = fork();

	if (pid < 0) {
        syslog(LOG_INFO, "Proces glowny");
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	sid = setsid();
	if (sid < 0) {

		exit(EXIT_FAILURE);
	}
	
	syslog(LOG_INFO, "Wczytano argumenty");
	ifUserInput = 0;
	
	/* Petla Demona */
	while (1) {
		pid = fork();
		if (pid < 0)
		{
			exit(EXIT_FAILURE);
		}
		if((sleep(time))==0 || ifUserInput == 1){
            syslog(LOG_INFO, "Demon sie obudzil");
        }
		if (pid == 0)
		{
			monitorCatalogue(path2,path1,rek,size);
            monitorDelete(path2,path1,rek);
			exit(EXIT_SUCCESS);
		}
		sleep(time);
	}
	closelog();
        while(wait(NULL) > 0);
	exit(EXIT_SUCCESS);
}
