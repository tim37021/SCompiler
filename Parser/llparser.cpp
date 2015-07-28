#include "llparser.h"
#include <fstream>
#include <rule.h>
#include <list>
#include <sstream>
#include <algorithm>
#include <cstdio>

using namespace std;


// Some helper function here

// trim from start
static inline std::string &ltrim(std::string &s) {
    size_t pos=s.find_first_not_of(" \t\f\v\n\r");
    if(pos!=std::string::npos)
        s.erase(s.begin(), s.begin()+pos);
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    size_t pos=s.find_last_not_of(" \t\f\v\n\r");
    if(pos!=std::string::npos)
        s.erase(s.begin()+pos+1, s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

static bool skipSeperator(SLexer::Lexer &l, SLexer::Token &t)
{
	do{
		if(l.hasNext())
			t=l.nextToken();
		else
			return false;
	}
	while(t.category==SLexer::Seperator);
	return true;
}


static void logging(ostream &out, int level, const string &tok)
{
	out<<string(level-1, ' ')<<level<<" "<<tok<<endl;
}

namespace SParser
{
	LLParser::LLParser(const char *filename, SLexer::Lexer *lex)
		: m_lexer(lex)
	{
		readTable(filename);
	}

	void LLParser::readTable(const char *filename)
	{
		ifstream ifs(filename);

		string line;
		if(!ifs)
			return;
			
		string entry;
		ifs>>entry;
		reset(entry.c_str());
			
		string non_ter, ter;
		while(ifs>>non_ter>>ter)
		{
			ifs.ignore();
			string rule;
			getline(ifs, rule);
			m_lltable[non_ter][ter]=trim(rule);
		}
	}

	void LLParser::step(ostream &log)
	{
		// Save current lexer state
		SLexer::Lexer lexer=*m_lexer;

		SLexer::Token t;
		bool eos=!skipSeperator(*m_lexer, t);

		map<string, map<string, string> >::iterator it;
		if(!isTerminal(it, m_stack.top().second))
		{
			// We must recover current state
			*m_lexer=lexer;
			string match=t.value;
			if(t.category==SLexer::Identifier) match="id";
			if(t.category==SLexer::Number) match="num";
			if(eos) match="$";

			// llparser little hack !!!!!!!
			if(m_stack.top().second=="ExprHelper")
				m_lexer->encodeExpr();

			auto it2=it->second.begin();
			if((it2=it->second.find(match))!=it->second.cend())
			{
				int level=m_stack.top().first;
				
				logging(log, m_stack.top().first, m_stack.top().second);
				m_stack.pop();
				if(it2->second!="epsilon")
				{
					stringstream sstr(it2->second);
					string tok;
					//Expand this nonterminal
					
					list<string> tmp_list;
					// push the rule to the stack...
					while(sstr>>tok)
						tmp_list.push_front(tok);
					for(string &t: tmp_list)
						m_stack.push(make_pair<int, string>(level+1, t.c_str()));
				}else
					logging(log, level+1, "epsilon");
			}else
				throw SyntaxError(string("Syntax error around ")+t.value+" Terminal="+m_stack.top().second);
		}else
		{
			if(m_stack.top().second=="id")
			{
				if(t.category!=SLexer::Identifier)
					throw SyntaxError(string("Failed to match identifier around ")+t.value+"->"+t.category->name);
				logging(log, m_stack.top().first, m_stack.top().second);
				logging(log, m_stack.top().first+1, t.value);
			}else if(m_stack.top().second=="num")
			{
				if(t.category!=SLexer::Number)
					throw SyntaxError(string("Failed to match number around ")+t.value);
				logging(log, m_stack.top().first, m_stack.top().second);
				logging(log, m_stack.top().first+1, t.value);
				
			}else if(m_stack.top().second=="$")
			{
			    if(!eos)
			        throw SyntaxError("Ending expected but input still continues");
			    logging(log, m_stack.top().first, m_stack.top().second);
			}else{
				if(m_stack.top().second!=t.value)
					throw SyntaxError(string("Failed to match token! expected '")+m_stack.top().second+"' input '"+t.value+"'");
				logging(log, m_stack.top().first, m_stack.top().second);
			}
			m_stack.pop();
		}
	}

	void LLParser::reset(const char *start_symbol)
	{
		while (!m_stack.empty())
    		m_stack.pop();
    	m_stack.push(make_pair<int, string>(1, "$"));
		m_stack.push(make_pair<int, string>(1, start_symbol));
	}
}