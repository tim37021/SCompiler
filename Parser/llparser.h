#ifndef _LLPARSER_H_
#define _LLPARSER_H_

#include <map>
#include <utility>
#include <string>
#include <lexer.h>
#include <stdexcept>
#include <iostream>
#include <stack>

namespace SParser
{
	typedef std::pair<int, std::string> Element;

	class SyntaxError: public std::runtime_error
	{
	public:
		explicit SyntaxError(const std::string &what_): std::runtime_error(what_){}
	};

	class LLParser
	{
	public:
		LLParser(const char *filename, SLexer::Lexer *lex);
		void reset(const char *start_symbol);
		void step(std::ostream &log);
		bool isOver() { return m_stack.empty(); }
	private:
		void readTable(const char *filename);
		bool isTerminal(std::map<std::string, std::map<std::string, std::string> >::iterator &it, 
			const std::string &tok)
		{ return (it=m_lltable.find(tok))==m_lltable.cend(); }

		SLexer::Lexer *m_lexer;
		std::stack<Element> m_stack;
		std::map<std::string, std::map<std::string, std::string> > m_lltable;
	};
};

#endif