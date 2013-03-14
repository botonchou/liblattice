#SHELL := /bin/bash
CXX=g++
CFLAGS=-g -w
all: indexer retrieve
SRC=\
    lattice.cpp\
    sqlite_wrapper.cpp\
    utility.cpp\
    index_builder.cpp\

INCLUDE=$(SRC:.cpp=.h)
LIBS=$(INCLUDE:.h=.o)
LD_LIBRARY= -lsqlite3\
	    -lpthread\
	    -ldl\
	    -lcmdparser\
	    -larray

INCLUDE_PATH=\
	     -I ./\
	     -I /usr/local/src/sqlite3 \
	     -I /usr/local/boost_1_53_0/\
	     -I /usr/local/boton/include/

LIBRARY_PATH=-L/usr/local/boost_1_53_0/libs/\
	     -L/usr/local/boton/lib/

indexer: $(LIBS) $(INCLUDE) indexer.cpp
	$(CXX) $(CFLAGS) -o indexer indexer.cpp $(LIBS) $(INCLUDE_PATH) $(LD_LIBRARY) $(LIBRARY_PATH) 
	@ctags -R *
retrieve: $(LIBS) $(INCLUDE) retrieve.cpp
	$(CXX) $(CFLAGS) -o retrieve retrieve.cpp $(LIBS) $(INCLUDE_PATH) $(LD_LIBRARY) $(LIBRARY_PATH)
	@ctags -R *
test: test.cpp
	$(CXX) -o test test.cpp
%.o: %.cpp %.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE_PATH)
.PHONY:
clean:
	rm -rf indexer retrieve *.o
