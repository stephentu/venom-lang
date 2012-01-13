CXX = g++
LEX = flex
YACC = bison

-include conf/rules.mk

ifdef DEBUG
CXXFLAGS_REST = -g
else
CXXFLAGS_REST = -O3 -DNDEBUG
endif

CXXFLAGS = -Wall -Werror -I$(PWD) $(CXXFLAGS_REST) 
LDFLAGS = 

PWD := $(shell pwd)

# TODO: hack for now
SRCFILES := $(shell find . -path './test'  -prune -o -type f \( -name '*.cc' ! -name 'venom.cc' \) -print)
OBJFILES := $(patsubst %.cc,%.o,$(subst ./,,$(SRCFILES)))

GENOBJFILES = parser/parser.o parser/scanner.o
ALLOBJFILES := $(sort $(OBJFILES) $(GENOBJFILES))

DEPS := $(ALLOBJFILES:.o=.d)

all: venom

.PHONY: test
test: test-compile

.PHONY: test-compile
test-compile: test/venom-test
	test/venom-test

# Generate scanner and parser

parser/parser.cc: parser/parser.yy
	$(YACC) -v -o $@ --defines=parser/parser.h $< 

parser/scanner.cc: parser/scanner.ll
	$(LEX) -o $@ $<

# Implicit rule to compile c++ files

%.o: %.cc
	$(CXX) $(CXXFLAGS) -MD -c -o $@ $<

-include $(DEPS)

# Link executable

venom: $(GENOBJFILES) $(ALLOBJFILES) venom.o
	$(CXX) $(LDFLAGS) -o $@ $(ALLOBJFILES) venom.o 

# TODO: Only link in the necessary obj files
# TODO: Better way of generating executables (very manual now)

test/venom-test: $(GENOBJFILES) $(ALLOBJFILES) test/venom-test.o
	$(CXX) $(LDFLAGS) -o $@ $(ALLOBJFILES) test/venom-test.o

GENFILES = parser.cc parser.h scanner.cc location.hh position.hh stack.hh
GENERATED_SRCS = $(addprefix parser/,$(GENFILES))

BINARIES = venom test/venom-test 
BINARIES_OBJ = $(addsuffix .o,$(BINARIES))
BINARIES_DEPS = $(addsuffix .d,$(BINARIES))

.PHONY: clean
clean:
	rm -f $(ALLOBJFILES) $(BINARIES_OBJ)
	rm -f $(GENERATED_SRCS)
	rm -f $(DEPS) $(BINARIES_DEPS)
	rm -f $(BINARIES)

.PHONY: count-lines
count-lines: clean
	cloc . --force-lang='yacc,yy' --force-lang='lex,ll'
