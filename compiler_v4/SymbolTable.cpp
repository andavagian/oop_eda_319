#include "SymbolTable.hpp"

#include <stdexcept>

void SymbolTable::enterScope()
{
	scopes.emplace_back();
}

void SymbolTable::exitScope()
{
	if (!scopes.empty())
		scopes.pop_back();
}

int SymbolTable::declare(const std::string& name)
{
	if (scopes.empty())
		scopes.emplace_back();
	scopes.back()[name] = nextAddr;
	return nextAddr++;
}

int SymbolTable::lookup(const std::string& name) const
{
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
	{
		auto found = it->find(name);
		if (found != it->end())
			return found->second;
	}
	throw std::runtime_error("Undefined variable: " + name);
}
