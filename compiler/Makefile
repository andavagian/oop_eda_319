NAME     = ast
CXX      = c++
CXXFLAGS = -Wall -Wextra -std=c++11

SRC = main.cpp \
      ../parser_v3/lexer.cpp \
      ../parser_v3/tokenizer.cpp \
      ../parser_v3/parser.cpp \
      compile.cpp \
      execute.cpp

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
