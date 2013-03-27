VULCAN_ROOT=/home/boton/vulcan

CC=gcc
CXX=g++
CFLAGS=

CPPFLAGS=-Wall -fstrict-aliasing $(CFLAGS) -I include/ -I /usr/local/boton/include/ -I $(VULCAN_ROOT)

SOURCES=sqlite_wrapper.cpp\
	utility.cpp\
	index_builder.cpp\
	lattice.cpp
	
 
all: indexer retrieve ctags

vpath %.h include/
vpath %.cpp src/

OBJ=$(addprefix obj/,$(SOURCES:.cpp=.o))

LIBRARY= -lsqlite3\
	 -lpthread\
	 -ldl\
	 -lcmdparser\
	 -larray\
	 -lvulcan_htk\
	 -lvulcan_common

LIBRARY_PATH=-L/usr/local/boton/lib/ -L$(VULCAN_ROOT)/htk/ -L$(VULCAN_ROOT)/common/

indexer: $(OBJ) indexer.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LIBRARY_PATH) $(LIBRARY) 
retrieve: $(OBJ) retrieve.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LIBRARY_PATH) $(LIBRARY)
test: $(OBJ) test.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LIBRARY_PATH) $(LIBRARY)
ctags:
	@ctags -R *

obj/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -o $@ -c $<

obj/%.d: %.cpp
	@$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,obj/\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

-include $(addprefix obj/,$(subst .cpp,.d,$(SOURCES)))

.PHONY:
clean:
	rm -rf indexer retrieve obj/*
