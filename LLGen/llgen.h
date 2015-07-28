#ifndef __SLLGEN_LLGEN_H_
#define __SLLGEN_LLGEN_H_

#include "symbol_table.h"
#include <string>
#include <utility>
#include <list>
#include <map>
#include <vector>
#include <stdexcept>

namespace SLLGen
{
	enum UnaryOperation
	{
		UO_MINUS,
		UO_NOT
	};

	enum BinaryOperation
	{
		BO_ADD,
		BO_SUB,
		BO_MUL,
		BO_DIV,
	
		BO_EQ,
		BO_NE,
		BO_GT,
		BO_GE,
		BO_LT,
		BO_LE,
		
		BO_AND,
		BO_OR
	};

	struct InvalidOperation: public std::runtime_error
	{
		InvalidOperation(): std::runtime_error("Invalid Operation"){}
		InvalidOperation(const std::string what): std::runtime_error(what){}
	};

	struct UndefinedSymbol: public std::runtime_error
	{
		UndefinedSymbol(): std::runtime_error("Undefined Symbol"){}
		UndefinedSymbol(const std::string what): std::runtime_error(what){}
	};

	struct TypeError: public std::runtime_error
	{
		TypeError(): std::runtime_error("Type Error"){}
		TypeError(const std::string what): std::runtime_error(what){}
	};

	enum Type
	{
		T_VOID,
		T_CHAR,
		T_INT,
		T_FLOAT,
		T_DOUBLE,

		T_VOIDPTR,
		T_CHARPTR,
		T_INTPTR,
		T_FLOATPTR,
		T_DOUBLEPTR
	};

	struct IfClause
	{
		std::string cond, beforeLabel, trueLabel, falseLabel;
		std::string trueClause;
		std::string falseClause;
	};

	typedef int ClauseStep;

	class LLGen
	{
	public:
		LLGen();
		void beginFunction(Type rntType, const std::string &name, const std::list<std::pair<Type, std::string > > &params=std::list<std::pair<Type, std::string > >());

		void addVariableDecl(Type type, const std::string &name, int arraySize=0, bool is_arg=false, bool no_insert=false);
		void addFunctionDecl(Type rntType, const std::string &name, const std::list<std::pair<Type, std::string > > &params=std::list<std::pair<Type, std::string > >());

		std::string addUnaryExpr(UnaryOperation, const std::string &rhs);
		std::string addBinaryExpr(const std::string &lhs, BinaryOperation, const std::string &rhs);
		std::string addAssignStmt(const std::string &lhs, const std::string &rhs);
		std::string addFunctionCall(const std::string &func, const std::list<std::string> &params);
		void addPrintStmt(const std::string &rhs);
		void beginIfClause(const std::string &cond);
		void elseClause();
		void endIfClause();
		void beginWhileCondEvalution();
		void beginWhile(const std::string &cond);
		void endWhile();
		void addBreakStmt();
		void addReturnStmt(const std::string &rhs);
		std::string loadMemory(const std::string &name, const std::string &sub="");


        SymbolTable &getTable()
        { return m_table; }
		// This function will reset temp variable
		void endFunction();

		std::string getLL() const
		{ return m_ll; }
		
		std::string getWarningLog() const;
		
	private:
		std::string getSymbolType(const std::string &name);
		std::string allocTemp(const std::string &type);
		std::string getElementType(const std::string &type);
		int isTempVar(const std::string &name);
		bool isPointerType(const std::string &type);
		std::string ref(const std::string &type);
		bool isSymbol(const std::string &name) const;
		std::string genBasicOperation_i32(BinaryOperation, const std::string &lhs, const std::string &rhs, const std::string &type, std::string &outType);
		std::string genBasicOperation_floating(BinaryOperation, const std::string &lhs, const std::string &rhs, const std::string &type, std::string &outType);
		void prepare(const std::string &name, std::string &outNameValue,
			std::string &outType, bool dontLoad=false);
		bool isArray(const std::string &type);

		std::string castType(const std::string &type, const std::string &var, const std::string &targetType);

		SymbolTable m_table;
		std::map<std::string, std::string> m_recentData;
		std::map<std::string, std::string> m_addressedParameter;
		std::vector<IfClause> m_ifClause;
		std::vector<ClauseStep> m_clauseStep;
		std::vector<bool> m_endBlock;
		int m_tempCounter;
		std::string m_beforeLabel;
		std::string m_ll;
		std::string m_funcLL;
		std::string *m_currentTargetLL;
		std::string m_warningLog;
		Symbol m_currentFuncSymbol;
	};

}

#endif