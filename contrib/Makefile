all: dbconvert

CXX         = $(shell root-config --cxx)
ROOTCFLAGS  = $(shell root-config --cflags)
LD          = $(shell root-config --ld)
LDFLAGS     = $(shell root-config --ldflags)

INCDIRS     = ../src ../hana_decode
PKGINCLUDES = $(addprefix -I, $(INCDIRS))
CXXFLAGS    = -g -O2

CXXFLAGS   += $(ROOTCFLAGS) $(PKGINCLUDES)
INCLUDES    = -I$(shell root-config --incdir) $(PKGINCLUDES)
LIBS        = $(shell root-config --libs) -L.. -lHallA -ldc

dbconvert:	dbconvert.o
		$(LD) $(LDFLAGS) $(LIBS) -o $@ $^

clean:
		rm -f dbconvert dbconvert.o

%.o:		%.cxx Makefile
		$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: all clean
