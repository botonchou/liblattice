#SHELL := /bin/bash
CXX=g++
CFLAGS=-g -w
all: main
SRC=\
    lattice.cpp\
    sqlite_wrapper.cpp\
    utility.cpp\
    index_builder.cpp\

INCLUDE=$(SRC:.cpp=.h)
LIBS=$(INCLUDE:.h=.o)

INCLUDE_PATH=\
	     -I ./\
	     -I /usr/local/src/sqlite3 \
	     -I /usr/local/boost_1_53_0/

LIBRARY_PATH=-L/usr/local/boost_1_53_0/libs/

main: $(LIBS) $(INCLUDE) main.cpp
	$(CXX) $(CFLAGS) -o indexer main.cpp $(LIBS) $(INCLUDE_PATH) -lsqlite3 -lpthread -ldl $(LIBRARY_PATH) 
	@ctags -R *
test: test.cpp
	$(CXX) -o test test.cpp
%.o: %.cpp %.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE_PATH)
.PHONY:
clean:
	rm -rf indexer *.o
