compileprojekt: Projekt.c
	gcc Projekt.c -o Projekt -lssl -lcrypto
prepareprojekt:
	mkdir ProjektPrzyklad
	touch ./ProjektPrzyklad/plik1
	touch ./ProjektPrzyklad/plik1.txt
	mkdir ./ProjektPrzyklad/KatalogPrzyklad
	mkdir ./ProjektPrzyklad/KatalogbezZawartosci
	touch ./ProjektPrzyklad/KatalogPrzyklad/plik2.txt
