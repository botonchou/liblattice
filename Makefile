CC=g++
CXX=g++
CPPFLAGS=-g -w -I include/

SOURCES=lattice.cpp\
	sqlite_wrapper.cpp\
	utility.cpp\
	index_builder.cpp

all: indexer retrieve

vpath %.h include/
vpath %.cpp src/

OBJ=$(addprefix obj/,$(SOURCES:.cpp=.o))

LD_LIBRARY_PATH= -lsqlite3\
		 -lpthread\
		 -ldl\
		 -lcmdparser\
		 -larray

LIBRARY_PATH=-L/usr/local/boton/lib/

indexer: $(OBJ) indexer.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LD_LIBRARY_PATH) $(LIBRARY_PATH) 
	@ctags -R *
retrieve: $(OBJ) retrieve.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LD_LIBRARY_PATH) $(LIBRARY_PATH)
	@ctags -R *
test: $(OBJ) test.cpp
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LD_LIBRARY_PATH) $(LIBRARY_PATH)
	@ctags -R *

obj/%.o: %.cpp
	$(CC) $(CPPFLAGS) -o $@ -c $<

obj/%.d: %.cpp
	@$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,obj/\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

-include $(addprefix obj/,$(subst .cpp,.d,$(SOURCES)))

.PHONY:
clean:
	rm -rf indexer retrieve obj/*
