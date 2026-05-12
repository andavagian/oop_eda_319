#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class SymbolTable
{
	std::vector<std::unordered_map<std::string, int>> scopes;
	int nextAddr = 0;

public:
	void enterScope();
	void exitScope();
	int  declare(const std::string& name); // assigns nextAddr++, returns address
	int  lookup(const std::string& name) const; // top-to-bottom search, throws if absent
};
