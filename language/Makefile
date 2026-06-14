NAME     = compilerv4
CXX      = c++
CXXFLAGS = -Wall -Wextra -std=c++17 -I.

SRC = SymbolTable/SymbolTable.cpp \
      Tokenizer/tokenizer.cpp \
      Parser/parser.cpp \
      IR/IR.cpp \
      Compiler/Assembler.cpp \
      Compiler/MachineCode.cpp \
      VirtualMachine/CPU.cpp \
      Runner/main.cpp \
      ../parser_v3/lexer.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
