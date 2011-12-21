CXX = g++
LEX = flex
YACC = bison

PWD := $(shell pwd)

CXXFLAGS = -Wall -g  -I$(PWD)
LDFLAGS = 

HEADERS = driver.h parser.h scanner.h expression.h \
    y.tab.h

all: venom

# Generate scanner and parser

parser.cc: parser.yy
	$(YACC) -o parser.cc --defines=parser.h parser.yy

scanner.cc: scanner.ll
	$(LEX) -o scanner.cc scanner.ll

# Implicit rule to compile c++ files

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link executable

venom: venom.o parser.o scanner.o driver.o
	$(CXX) $(LDFLAGS) -o $@ venom.o parser.o scanner.o driver.o

GENERATED = parser.cc parser.h scanner.cc location.hh position.hh stack.hh

clean:
	rm -f venom *.o $(GENERATED)
