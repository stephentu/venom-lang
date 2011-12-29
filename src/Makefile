CXX = g++
LEX = flex
YACC = bison

CXXFLAGS = -Wall -g -I$(PWD)
LDFLAGS = 

PWD := $(shell pwd)

# TODO: hack for now
SRCFILES := $(shell find . -path './test'  -prune -o -type f \( -name '*.cc' ! -name 'venom.cc' \) -print)
OBJFILES := $(patsubst %.cc,%.o,$(subst ./,,$(SRCFILES)))

GENOBJFILES = parser/parser.o parser/scanner.o
ALLOBJFILES := $(sort $(OBJFILES) $(GENOBJFILES))

all: venom

.PHONY: test
test: test/venom-test
	test/venom-test

# Generate scanner and parser

parser/parser.cc: parser/parser.yy
	$(YACC) -v -o $@ --defines=parser/parser.h $< 

parser/scanner.cc: parser/scanner.ll
	$(LEX) -o $@ $<

# Implicit rule to compile c++ files

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link executable

venom: $(GENOBJFILES) $(ALLOBJFILES) venom.o
	$(CXX) $(LDFLAGS) -o $@ $(ALLOBJFILES) venom.o 

test/venom-test: $(GENOBJFILES) $(ALLOBJFILES) test/venom-test.o
	$(CXX) $(LDFLAGS) -o $@ $(ALLOBJFILES) test/venom-test.o

GENFILES = parser.cc parser.h scanner.cc location.hh position.hh stack.hh
GENERATED = $(addprefix parser/,$(GENFILES))

clean:
	rm -f $(ALLOBJFILES) venom.o test/venom-test.o
	rm -f $(GENERATED)
	rm -f venom
	rm -f test/venom-test
