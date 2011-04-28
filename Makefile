CXX = g++ -fPIC

all: directoryServer

directoryServer : directoryServer.o
	$(CXX) -o $@ $@.o -lpthread

%.o: %.cc
	@echo 'Building $@ from $<'
	$(CXX) -g -o $@ -c -I. $<

clean:
	rm -f *.o directoryServer

