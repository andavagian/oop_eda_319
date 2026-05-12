NAME1    = Compiler
NAME2    = VirtualMachine
CXX      = c++
CXXFLAGS = -Wall -Wextra -std=c++17 -I.

COMMON_SRC = SymbolTable.cpp \
             tokenizer.cpp \
             parser.cpp \
             compile.cpp \
             execute.cpp \
             ../parser_v3/lexer.cpp

OBJ_COMMON = $(COMMON_SRC:.cpp=.o)
OBJ1       = $(OBJ_COMMON) mainCompiler.o
OBJ2       = $(OBJ_COMMON) mainVM.o

all: $(NAME1) $(NAME2)

$(NAME1): $(OBJ1)
	$(CXX) $(CXXFLAGS) $(OBJ1) -o $(NAME1)

$(NAME2): $(OBJ2)
	$(CXX) $(CXXFLAGS) $(OBJ2) -o $(NAME2)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_COMMON) mainCompiler.o mainVM.o

fclean: clean
	rm -f $(NAME1) $(NAME2)

re: fclean all

.PHONY: all clean fclean re
