mytree: main.o mytree_util.o
	gcc -g -o mytree main.o mytree_util.o 

main.o: main.c
	gcc -c -g main.c

mytree_util.o: mytree_util.h mytree_util.c
	gcc -c -g mytree_util.c

.PHONY: clean
clean:
	rm *.o mytree
