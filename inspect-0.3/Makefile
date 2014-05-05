

all:  scheduler  


scheduler:
	make -C ./src
	mv ./src/inspect .
	cp inspect ../bin

distclean:
	make -C ./src clean
	rm -f target 
	rm -f inspect
	rm -f ../bin/inspect

clean:
	make -C ./src/  clean
	rm -f target
	rm -f inspect
	rm -f ../bin/inspect



