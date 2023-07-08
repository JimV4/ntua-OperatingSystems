all: treeTest treeTestSig treePipes

treeTest: treeTest.o tree.o proc-common.o
	gcc treeTest.o tree.o proc-common.o -o treeTest

treeTestSig: treeTestSig.o tree.o proc-common.o
	gcc treeTestSig.o tree.o proc-common.o -o treeTestSig

treePipes: treePipes.o tree.o proc-common.o
	gcc treePipes.o tree.o proc-common.o -o treePipes

tree.o: tree.c
	gcc -Wall -c tree.c

proc-common.o: proc-common.c
	gcc -Wall -c proc-common.c

treeTest.o: treeTest.c
	gcc -Wall -c treeTest.c

treeTestSig.o: treeTestSig.c
	gcc -Wall -c treeTestSig.c

treePipes.o: treePipes.c
	gcc -Wall -c treePipes.c

clean:
	rm ./*.o treeTest treeTestSig treePipes