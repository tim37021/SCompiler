#ifndef __SLLGEN_SYMBOL_TABLE_H_
#define __SLLGEN_SYMBOL_TABLE_H_

#include <string>
#include <map>
#include <list>
#include <stdexcept>
#include <iostream>
#include "symbol.h"

namespace SLLGen
{
	class SymbolRedeclare: std::exception
	{
	public:
		SymbolRedeclare(): std::exception(){}
	};

	class Scope
	{
	public:
		Scope(Scope *parent);
		~Scope();

		void insert(const std::string &, const Symbol &);
		bool lookUp(const std::string &, Symbol &) const;
		bool getUniqueName(const std::string &name, std::string &out, int level=0) const;

		Scope *addChild();
		Scope *getParent() const
		{ return m_parent; }
	    std::list<Scope *> &getChildren()
	    { return m_children; }

		std::map<std::string, Symbol> &getSymbols()
		{ return m_symbols; }
	private:
		Scope *m_parent;
		std::list<Scope *> m_children;
		std::map<std::string, Symbol> m_symbols;
		static int counter;
	};

	class SymbolTable
	{
	public:
		SymbolTable();
		~SymbolTable();

		void push();
		void pop();
		bool isInGlobal() const
		{ return m_globalScope==m_currentScope; }

		void insert(const std::string &, const Symbol &);
		bool lookUp(const std::string &, Symbol &) const;
		bool getUniqueName(const std::string &name, std::string &out) const;

		Scope *getCurrentScope() const
		{ return m_currentScope; }

		void clear();
		void dump(std::ostream &) const;
	private:
		Scope *m_globalScope;
		Scope *m_currentScope;
	};
}

#endif