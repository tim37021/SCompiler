#include <iostream>
#include <fstream>

#include <lexer.h>
#include <rule.h>

#include "llparser.h"

using namespace std;
using namespace SLexer;
using namespace SParser;

std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if(!ifs)
		exit(EXIT_FAILURE);
	return std::string( (std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

int main(int argc, char *argv[])
{
	// llparser filename lltable
    if(argc!=3)
    {
        fprintf(stderr, "llparser <input> <lltable>\n");
        return 1;
    }

	PrecedenceList precedence=
	{Comment, Keyword, Identifier, Operator, Number, SpecialSymbol, Number, Char, Seperator, Error};
	Lexer myLex(readfile(argv[1]).c_str(), precedence);

	LLParser parser(argv[2], &myLex);

	while(!parser.isOver())
	{
		try{
			parser.step(cout);
		}catch(SyntaxError &e)
		{
			cerr<<e.what()<<endl;
			cerr<<myLex.getCode()<<endl;
			return 1;
		}
	}

	return 0;
}
