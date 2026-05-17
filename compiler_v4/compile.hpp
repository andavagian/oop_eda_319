#pragma once

#include "BlockNode.hpp"
#include "FuncNode.hpp"
#include "VM.hpp"
#include <memory>
#include <vector>

void compile(
	const std::vector<std::unique_ptr<FuncNode>>& funcs,
	const BlockNode*                              mainBlock,
	VM&                                           vm);
