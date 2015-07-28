#include "symbol_table.h"
#include <vector>
#include <utility>
#include <cctype>

namespace SLLGen
{
	int Scope::counter=0;
	Scope::Scope(Scope *parent):
		m_parent(parent)
	{

	}

	Scope::~Scope()
	{
		for(auto child: m_children)
			delete child;
	}

	void Scope::insert(const std::string &name, const Symbol &var)
	{
		Symbol symbol;
		symbol=var;
		symbol.uniqueName=(m_parent?"%": "@")+name;
		if(!(name[0]>='0'&&name[0]<='9'))
			symbol.uniqueName+=std::to_string(counter++);
		auto it=m_symbols.find(name);
		if(it==m_symbols.cend())
			m_symbols[name]=symbol;
		else if(it->second.type!=var.type||it->second.paramTypes!=var.paramTypes)
			throw SymbolRedeclare();
	}

	bool Scope::lookUp(const std::string &name, Symbol &out_var) const
	{
		auto it=m_symbols.find(name);
		if(it!=m_symbols.cend())
		{
			out_var=it->second;
			return true;
		}

		if(m_parent)
			return m_parent->lookUp(name, out_var);
		else
			return false;
	}

	bool Scope::getUniqueName(const std::string &name, std::string &out_name, int level) const
	{
		Symbol out_var;
		bool result=lookUp(name, out_var);
		out_name=out_var.uniqueName;
	}

	Scope *Scope::addChild()
	{
		Scope *new_scope = new Scope(this);
		m_children.push_back(new_scope);
		return new_scope;
	}

	SymbolTable::SymbolTable()
		: m_globalScope(new Scope(nullptr))
	{
		m_currentScope=m_globalScope;
	}

	SymbolTable::~SymbolTable()
	{
		delete m_globalScope;
	}

	void SymbolTable::push()
	{
		m_currentScope=m_currentScope->addChild();
	}

	void SymbolTable::pop()
	{
		if(m_currentScope!=m_globalScope)
			m_currentScope=m_currentScope->getParent();
	}

	void SymbolTable::insert(const std::string &name, const Symbol &var)
	{
		m_currentScope->insert(name, var);
	}

	bool SymbolTable::lookUp(const std::string &name, Symbol &var) const
	{
		return m_currentScope->lookUp(name, var);
	}

	void SymbolTable::clear()
	{
		delete m_globalScope;
		m_globalScope = new Scope(nullptr);
		m_currentScope = m_globalScope;
	}

	bool SymbolTable::getUniqueName(const std::string &name, std::string &out) const
	{
		out="";
		return m_currentScope->getUniqueName(name, out);
	}
	
	void SymbolTable::dump(std::ostream &out) const
	{
	    std::vector<std::pair<Scope *, int> > stk;
	    
	    stk.push_back({m_globalScope, 0});
	    
	    while(!stk.empty())
	    {
	        std::pair<Scope *, int> top=stk.back(); stk.pop_back();
	        
	        for(auto &p: top.first->getSymbols())
	        {
	            if(!isdigit(p.second.name[0]))
	                out<<top.second<<"\t"<<p.second.ctype<<"\t"<<p.second.name<<std::endl;
	        }
	        for(auto &child: top.first->getChildren())
    	        stk.push_back({child, top.second+1});
	    }
	}
}