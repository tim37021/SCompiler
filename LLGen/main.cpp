#include "llgen.h"
#include <fstream>
#include <iostream>
#include <vector>

using namespace SLLGen;
using namespace std;


struct ParseTreeNode
{
public:
	ParseTreeNode(): parent(nullptr){}
	ParseTreeNode(ParseTreeNode *p, const string &val)
	: value(val), parent(p){}
	string value;
	ParseTreeNode *parent;
	vector<ParseTreeNode> children;
};

ParseTreeNode root;
LLGen llg;


string ExprHelper(const ParseTreeNode &node);

void load_parse_tree(const char *filename)
{
	int depth;
	string nodeValue;
	root.children.clear();

	ParseTreeNode *current=&root;
	int last=0;

	ifstream ifs(filename);
	while(ifs>>depth)
	{
		ifs.ignore();
		getline(ifs, nodeValue);
		
		while(last>=depth)
		{
			current=current->parent;
			last--;
		}
		current->children.push_back({current, nodeValue});
		current=&current->children.back();
		last=depth;
	}

	ifs.close();
}


Type Type_term(const ParseTreeNode &node)
{
	if(node.children[0].value=="int")
		return T_INT;
	if(node.children[0].value=="char")
		return T_CHAR;
	if(node.children[0].value=="float")
		return T_FLOAT;
	if(node.children[0].value=="double")
		return T_DOUBLE;
}

void VarDecl_(Type type, const string &id, const ParseTreeNode &node)
{
	if(node.children[0].value==";")
		llg.addVariableDecl(type, id);
	if(node.children[0].value=="[")
	{
		int num=stoi(node.children[1].children[0].value);
		llg.addVariableDecl(type, id, num);
	}
}

void DeclList_(const ParseTreeNode &);
void VarDeclList(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return;
	DeclList_(node.children[0]);
	VarDeclList(node.children[1]);
}


BinaryOperation BinOp(const ParseTreeNode &node)
{
	if(node.children[0].value=="+")
		return BO_ADD;
	if(node.children[0].value=="-")
		return BO_SUB;
	if(node.children[0].value=="*")
		return BO_MUL;
	if(node.children[0].value=="/")
		return BO_DIV;

	if(node.children[0].value=="==")
		return BO_EQ;
	if(node.children[0].value=="!=")
		return BO_NE;
	if(node.children[0].value==">")
		return BO_GT;
	if(node.children[0].value==">=")
		return BO_GE;
	if(node.children[0].value=="<")
		return BO_LT;
	if(node.children[0].value=="<=")
		return BO_LE;
	if(node.children[0].value=="&&")
	    return BO_AND;
	if(node.children[0].value=="||")
	    return BO_OR;  
}

UnaryOperation UnaryOp(const ParseTreeNode &node)
{
	if(node.children[0].value=="-")
		return UO_MINUS;
	if(node.children[0].value=="!")
		return UO_NOT;
}

string Expr(const ParseTreeNode &);
string Expr_(string lhs, const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return lhs;
	BinaryOperation binop=BinOp(node.children[0]);
	string rhs=Expr(node.children[1]);
	return llg.addBinaryExpr(lhs, binop, rhs);
}

list<string> ExprListTail(const ParseTreeNode &);
list<string> ExprListTail_(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return {};
	return ExprListTail(node.children[1]);
}

list<string> ExprListTail(const ParseTreeNode &node)
{
	list<string> result;
	result.push_back(ExprHelper(node.children[0]));
	const list<string> &result2=ExprListTail_(node.children[1]);
	result.insert(result.end(), result2.begin(), result2.end());
	return result;
}

list<string> ExprList(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return {};
	return ExprListTail(node.children[0]);
}

string ExprArrayTail(const string &lhs, const string &sub, const ParseTreeNode &node)
{
	// epsilon production by expr'
	if(node.children[0].value=="epsilon")
		return llg.loadMemory(lhs, sub);
	if(node.children[0].value=="=")
	{
		string rhs=Expr(node.children[1]);
		return llg.addAssignStmt(lhs+"["+sub+"]", rhs);
	}
	return Expr_(llg.loadMemory(lhs, sub), node.children[0]);
}

string ExprIdTail(const string &lhs, const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return llg.loadMemory(lhs);
	//Function call
	if(node.children[0].value=="(")
	{
		auto params=ExprList(node.children[1]);
		return llg.addFunctionCall(lhs, params);
	}
	//Array
	if(node.children[0].value=="[")
	{
		string index=ExprHelper(node.children[1]);
		return ExprArrayTail(lhs, index, node.children[3]);
	}
	if(node.children[0].value=="=")
	{
		string rhs=Expr(node.children[1]);
		return llg.addAssignStmt(lhs, rhs);
	}
	return Expr_(lhs, node.children[0]);
}

string Expr(const ParseTreeNode &node)
{
	string result;
	if(node.children[0].value=="UnaryOp")
	{
		UnaryOperation uop=UnaryOp(node.children[0]);
		result=llg.addUnaryExpr(uop, Expr(node.children[1]));
	}
	if(node.children[0].value=="(")
	{
		result=Expr_(ExprHelper(node.children[1]), node.children[3]);
	}

	if(node.children[0].value=="id")
	{
		result= ExprIdTail(node.children[0].children[0].value, node.children[1]);
	}
	if(node.children[0].value=="num")
	{
		result=Expr_(node.children[0].children[0].value, node.children[1]);
	}
	return result;
}

string ExprHelper(const ParseTreeNode &node)
{
	// This function will call expression solver
	return Expr(node.children[0]);
}

void Stmt(const ParseTreeNode &node);
void ElseClause(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return;
	if(node.children[0].value=="else")
	{
		llg.elseClause();
		Stmt(node.children[1]);
	}
}

void Block(const ParseTreeNode &);
void Stmt(const ParseTreeNode &node)
{
	if(node.children[0].value=="if")
	{
		string cond=ExprHelper(node.children[2]);
		if(cond=="1")
			Stmt(node.children[4]);
		else if(cond=="0")
			Stmt(node.children[5]);
		else if(cond!="1"&&cond!="0")
		{
			llg.beginIfClause(cond);
			Stmt(node.children[4]);
			ElseClause(node.children[5]);
			llg.endIfClause();
		}
	}

	if(node.children[0].value=="while")
	{
		llg.beginWhileCondEvalution();
		string cond=ExprHelper(node.children[2]);
		llg.beginWhile(cond);
		Stmt(node.children[4]);
		llg.endWhile();
	}

	if(node.children[0].value=="Block")
	{
		Block(node.children[0]);
	}
	if(node.children[0].value=="return")
	{
		llg.addReturnStmt(ExprHelper(node.children[1]));
	}
	if(node.children[0].value=="ExprHelper")
	{
		ExprHelper(node.children[0]);
	}

	if(node.children[0].value=="print")
	{
		llg.addPrintStmt(ExprHelper(node.children[2]));
	}
	
	if(node.children[0].value=="break")
	{
	    llg.addBreakStmt();
	}
}
void StmtList(const ParseTreeNode &);
void StmtList_(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return;
	StmtList(node.children[0]);
}

void StmtList(const ParseTreeNode &node)
{
	Stmt(node.children[0]);
	StmtList_(node.children[1]);
}

void Block(const ParseTreeNode &node)
{
	VarDeclList(node.children[1]);
	StmtList(node.children[2]);
}

bool ParamDecl_(const ParseTreeNode &node)
{
	return node.children[0].value!="epsilon";
}

pair<Type, string> ParamDecl(const ParseTreeNode &node)
{
	Type type=(Type)(Type_term(node.children[0])+(ParamDecl_(node.children[2])? T_CHARPTR-T_CHAR: 0));
	string id=node.children[1].children[0].value;
	return {type, id};
}


list<pair<Type, string > > ParamDeclListTail(const ParseTreeNode &node);
list<pair<Type, string > > ParamDeclListTail_(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return {};
	return ParamDeclListTail(node.children[1]);
}

list<pair<Type, string > > ParamDeclListTail(const ParseTreeNode &node)
{
	list<pair<Type, string > > result;
	result.push_back(ParamDecl(node.children[0]));
	const list<pair<Type, string > > &result2=ParamDeclListTail_(node.children[1]);
	result.insert(result.end(), result2.begin(), result2.end());
	return result;
}

list<pair<Type, string > > ParamDeclList(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return {};
	return ParamDeclListTail(node.children[0]);
}

void FuncDecl_(Type type, const string &id, const std::list<std::pair<Type, std::string > > &params, const ParseTreeNode &node)
{
	if(node.children[0].value==";")
	{
		// forward declaration
		llg.addFunctionDecl(type, id, params);
	}
	if(node.children[0].value=="Block")
	{
		llg.beginFunction(type, id, params);
		Block(node.children[0]);
		llg.endFunction();
	}
}

void FuncDecl(Type type, const string &id, const ParseTreeNode &node)
{
	auto params=ParamDeclList(node.children[1]);
	FuncDecl_(type, id, params, node.children[3]);
}

void Decl(Type type, const string &id, const ParseTreeNode &node)
{	
	if(node.children[0].value=="VarDecl'")
		VarDecl_(type, id, node.children[0]);
	if(node.children[0].value=="FunDecl")
		FuncDecl(type, id, node.children[0]);
	VarDecl_(type, id, node);
}


void DeclList_(const ParseTreeNode &node)
{
	Type type=Type_term(node.children[0]);
	string id=node.children[1].children[0].value;

	Decl(type, id, node.children[2]);
}

void DeclList(const ParseTreeNode &node)
{
	if(node.children[0].value=="epsilon")
		return;
	DeclList_(node.children[0]);
	DeclList(node.children[1]);
}

void compile(const ParseTreeNode &node)
{
	DeclList(node.children[0]);
}



int main(int argc, char *argv[])
{
	load_parse_tree("tree.txt");

	try{
		compile(root.children.front());
		cout<<llg.getLL();
		
		cerr<<llg.getWarningLog();
		llg.getTable().dump(cerr);
	}catch(runtime_error &e)
	{
		cerr<<e.what()<<endl;
	}
	return 0;
}