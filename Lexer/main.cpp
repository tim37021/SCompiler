#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "rule.h"

using namespace SLexer;

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
	if(argc!=2)
	{
		fprintf(stderr, "Usage: laxer\n");
		exit(EXIT_FAILURE);
	}


	PrecedenceList precedence=
	{Comment, Keyword, Identifier, Operator, Number, SpecialSymbol, Number, Char, Seperator, Error};

	Lexer myLex(readfile(argv[1]).c_str(), precedence);

	int line=1;

	printf("Line  1:\n");

	myLex.encodeExpr();

	while(myLex.hasNext())
	{
		const Token &t=myLex.nextToken();
		if(t.category!=Seperator&&t.category!=Comment)
			printf("\t<%s>:\t%s\n", t.category->name.c_str(), t.value.c_str());
		else if(t.value=="\n")
		{
			line++;
			printf("Line  %d:\n", line);
		}

	}
	
	return EXIT_SUCCESS;
}