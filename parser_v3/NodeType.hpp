#pragma once

enum class NodeType
{
	Num, Var, EofEx, OpBr, ClBr, Op,
	If, While, Else, Comp,
	Assign, Not, Semi,
	OpBody, ClBody, Type,
	Block, Ret, Func,
	Call, Comma
};

enum class Operator
{
	Add,
	Sub,
	Mult,
	Div
};
