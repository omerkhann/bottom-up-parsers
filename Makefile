CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

SRC := src/main.cpp src/grammar.cpp src/items.cpp src/parsing_table.cpp src/slr_parser.cpp src/lr1_parser.cpp src/stack.cpp src/tree.cpp
OBJ := $(SRC:.cpp=.o)

all: parser

parser: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) parser

.PHONY: all clean run run-conflict run-expr run-epsilon run-ambiguous run-rr run-dangling

# Run helper: override these on the CLI, e.g.
# make run GRAMMAR=input/expr.txt TOKENS="id + id * id"
GRAMMAR ?= input/grammar_with_conflict.txt
TOKENS ?= id = id
OUTDIR ?= output

run: parser
	./parser --grammar $(GRAMMAR) --tokens "$(TOKENS)" --outdir $(OUTDIR)

run-conflict:
	$(MAKE) run GRAMMAR=input/grammar_with_conflict.txt TOKENS="id = id"

run-expr:
	$(MAKE) run GRAMMAR=input/expr_grammar.txt TOKENS="id + id * id"

run-epsilon:
	$(MAKE) run GRAMMAR=input/epsilon_grammar.txt TOKENS="b b"

run-ambiguous:
	$(MAKE) run GRAMMAR=input/ambiguous_expr.txt TOKENS="id + id * id"

run-rr:
	$(MAKE) run GRAMMAR=input/reduce_reduce_conflict.txt TOKENS="a"

run-dangling:
	$(MAKE) run GRAMMAR=input/dangling_else.txt TOKENS="if true then other else other"

help:
	@echo "Compiler Construction Assignment 3 - Bottom-Up Parser Suite"
	@echo "========================================================================"
	@echo "Available targets:"
	@echo "  all           : Build the 'parser' executable (default)"
	@echo "  clean         : Remove object files and the executable"
	@echo "  help          : Show this help message"
	@echo ""
	@echo "Execution targets:"
	@echo "  run           : Run with default or custom args"
	@echo "                  Usage: make run GRAMMAR=path/to/g.txt TOKENS=\"id + id\""
	@echo "  run-expr      : Run simple expression grammar test"
	@echo "  run-conflict  : Run grammar with SLR(1) conflicts (resolvable by LR(1))"
	@echo "  run-epsilon   : Run grammar containing epsilon (@) productions"
	@echo "  run-dangling  : Run the 'Dangling Else' grammar test"
	@echo "  run-rr        : Run grammar demonstrating Reduce/Reduce conflicts"
	@echo "========================================================================"

.PHONY: all clean run run-conflict run-expr run-epsilon run-ambiguous run-rr run-dangling help