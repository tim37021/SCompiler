#ifndef _HW1_RULE_H_
#define _HW1_RULE_H_

#include "lexer.h"

namespace SLexer
{
	int FetchComment(const char *);
	int FetchIdentifier(const char *);
	int FetchKeyword(const char *);
	int FetchOperator(const char *);
	int FetchSpecialSymbol(const char *);
	int FetchNumber(const char *);
	int FetchChar(const char *);
	int FetchSeperator(const char *);
	int FetchError(const char *);

	extern const Category *Comment;
	extern const Category *Identifier;
	extern const Category *Keyword;
	extern const Category *Operator;
	extern const Category *SpecialSymbol;
	extern const Category *Number;
	extern const Category *Char;
	extern const Category *Seperator;
	extern const Category *Error;
}

#endif