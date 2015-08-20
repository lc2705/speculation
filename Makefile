a:main.o ac.o
	gcc -lpthread  -o ac main.o ac.o
	rm *.o *.h.gch
main.o:main.c ac.h
	gcc -g -c main.c ac.h
ac.o:ac.c ac.h
	gcc -g -c ac.c ac.h
clean:
	rm -f *.o ac
