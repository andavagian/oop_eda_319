#pragma once

#include "ASTNode.hpp"
#include "VM.hpp"

void compile(const ASTNode* root, VM& vm);
