CXX = g++
LEX = flex
YACC = bison

PWD = $(shell pwd)

CXXFLAGS = -Wall -g  -I$(PWD)
LDFLAGS = 

SRCFILES = $(shell find . -type f -name '*.cc')
OBJFILES = $(patsubst %.cc,%.o,$(SRCFILES))

all: venom

# Generate scanner and parser

parser.cc: parser.yy
	$(YACC) -v -o parser.cc --defines=parser.h parser.yy

scanner.cc: scanner.ll
	$(LEX) -o scanner.cc scanner.ll

# Implicit rule to compile c++ files

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: generated
generated: parser.o scanner.o

# Link executable

venom: generated $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ $(OBJFILES)

GENERATED = parser.cc parser.h scanner.cc location.hh position.hh stack.hh

clean:
	rm -f $(OBJFILES) 
	rm -f $(GENERATED)
	rm -f venom
