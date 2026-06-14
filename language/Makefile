NAME     = langc
CXX      = c++
CXXFLAGS = -Wall -Wextra -std=c++17 -I.

SRC = SymbolTable/SymbolTable.cpp \
      Tokenizer/Tokenizer.cpp \
      Parser/Parser.cpp \
      IR/IR.cpp \
      Compiler/Assembler.cpp \
      Compiler/MachineCode.cpp \
      Linker/Linker.cpp \
      VirtualMachine/CPU.cpp \
      VirtualMachine/Debugger.cpp \
      Runner/main.cpp \
      ../common/lexer.cpp

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
