#pragma once

#include "Tokenizer/Token.hpp"
#include <string>
#include <vector>

std::vector<Token> tokenize(const std::vector<std::string>& pieces);
