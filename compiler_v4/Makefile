NAME     = compiler
CXX      = c++
CXXFLAGS = -Wall -Wextra -std=c++17 -I.

SRC = SymbolTable.cpp \
      tokenizer.cpp \
      parser.cpp \
      IR.cpp \
      Assembler.cpp \
      MachineCode.cpp \
      CPU.cpp \
      main.cpp \
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
