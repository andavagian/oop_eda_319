#pragma once

#include "Token.hpp"
#include "Node.hpp"

#include <sstream>
#include <vector>

std::vector<std::string> lexer(std::stringstream& input);
std::vector<Token> tokenizer(const std::vector<std::string>& pieces);
Node* parser(std::vector<Token>& tokens, std::vector<int>& vars);
