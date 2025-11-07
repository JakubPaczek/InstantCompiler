CXX := g++
CXXFLAGS := -std=c++17 -Isrc/frontend -O2 -Wall -Wextra

FRONT := src/frontend
FRONT_OBJS := $(FRONT)/Parser.o $(FRONT)/Printer.o $(FRONT)/Absyn.o \
              $(FRONT)/Lexer.o $(FRONT)/Buffer.o

LIB := lib

all: insc_jvm insc_llvm $(LIB)/Runtime.class

$(LIB)/Runtime.class: $(LIB)/Runtime.java
	javac -d $(LIB) $(LIB)/Runtime.java

# bnfc make
$(FRONT_OBJS):
	$(MAKE) -C $(FRONT)

# jvm
insc_jvm: src/insc_jvm.cpp $(FRONT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# llvm
insc_llvm: src/insc_llvm.cpp $(FRONT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# clean
clean:
	$(MAKE) -C $(FRONT) clean 
	rm -f insc_jvm insc_llvm
