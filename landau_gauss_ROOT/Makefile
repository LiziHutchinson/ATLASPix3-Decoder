# Makefile for langau

##
## Shared lib
SHLIB = liblangau.so

##
##
INCLUDE = -I$(ROOTSYS)/include

##
## Source files
SOURCES = Langau.cxx

##
## Includes 
INCFILES = Langau.h

##
## Object files
OBJECTS = $(SOURCES:.cxx=.o)

##
## Base name of dictionary file
DICT = LangauCint

OS = $(shell uname)
ifeq ($(OS),Linux)
  ## Linux version
  SOFLAGS = -shared -Wl    
  SOMAKER = g++
  CCFLAGS = -fno-rtti -fno-exceptions
else
  ## OSF1 version
  SOFLAGS = -L/usr/lib/cmplrs/cxx -rpath /usr/lib/cmplrs/cxx \
  -expect_unresolved "*" -g0 -O2 -shared /usr/lib/cmplrs/cc/crt0.o \
  /usr/lib/cmplrs/cxx/_main.o -lcxxstd -lcxx -lexc -lots -lc
  SOMAKER = ld
  CXX = cxx
  CXXFLAGS = -O2
endif

##
## Needed for shared lib

all: $(SHLIB)

clean: 
	rm -f $(DICT).* *.o *~ $(SHLIB) LinkDef.h
##
## General rule how to compile
%.o:	%.cxx
	$(CXX) $(CCFLAGS) $(CXXFLAGS) $(INCLUDE) -c $<

$(OBJECTS): $(SOURCES) $(INCFILES)

$(DICT).o: $(INCFILES)
	create_cint $(DICT).cxx
	$(CXX) $(CCFLAGS) $(CXXFLAGS) $(INCLUDE) -c $(DICT).cxx

$(SHLIB): $(OBJECTS) $(DICT).o
	$(SOMAKER) $(CCFLAGS) $(SOFLAGS) -o $(SHLIB) $(OBJECTS) $(DICT).o

