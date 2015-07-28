#include "rule.h"
#include <cctype>
#include <cstring>

static bool IsSeperator(char);

static bool IsSeperator(char c)
{
	// non-alphabetic non-numberic
	return !isalpha(c)&&!isdigit(c)&c!='\'';
}

namespace SLexer
{
	const Category Comment_instance("Comment", FetchComment);
	const Category Identifier_instance("Identifier", FetchIdentifier);
	const Category Keyword_instance("Keyword", FetchKeyword);
	const Category Operator_instance("Operator", FetchOperator);
	const Category SpecialSymbol_instance("Special", FetchSpecialSymbol);
	const Category Number_instance("Number", FetchNumber);
	const Category Char_instance("Char", FetchChar);
	const Category Seperator_instance("Seperator", FetchSeperator);
	const Category Error_instance("Error", FetchError);

	const Category *Comment=&Comment_instance;
	const Category *Identifier=&Identifier_instance;
	const Category *Keyword=&Keyword_instance;
	const Category *Operator=&Operator_instance;
	const Category *SpecialSymbol=&SpecialSymbol_instance;
	const Category *Number=&Number_instance;
	const Category *Char=&Char_instance;
	const Category *Seperator=&Seperator_instance;
	const Category *Error=&Error_instance;

	int FetchComment(const char *str_seq)
	{
		int length=0;
		if(str_seq[0]!='/'||str_seq[1]!='/')
			return 0;
		for(; str_seq[length]!='\n'&&str_seq[length]!='\0'; length++);
		return length;
	}

	//the first world can't be numberic, else can be _, [a-z], [A-Z], [0-9]
	int FetchIdentifier(const char *str_seq)
	{
		int length=0;
		for(; str_seq[length]; length++)
		{
			if(length==0&&isdigit(str_seq[0]))
				break;
			if(!isdigit(str_seq[length])&&!isalpha(str_seq[length])&&str_seq[length]!='_')
				break;
		}
		if(IsSeperator(str_seq[length]))
			return length;
		else
			return 0;
	}

	int FetchKeyword(const char *str_seq)
	{
		static const char *keywords[]=
		{"int", "char", "float", "double", "return",
		 "if", "else", "while", "break", "for",
		 "print"};
		static int max_length=6;

		int length=FetchIdentifier(str_seq);

		for(int i=0; i<sizeof(keywords)/sizeof(keywords[0]); i++)
		{
			int compare_length=strlen(keywords[i]);
			compare_length=compare_length>length?compare_length: length;
			if(strncmp(keywords[i], str_seq, compare_length)==0)
				return length;
		}
		return 0;
	}

	int FetchOperator(const char *str_seq)
	{
		static const char *operators[]=
		{"=", "!", "+", "-", "*", "/", ".", "==", "!=", "<", ">", "<=", ">=", "&&", "||"};
		for(int i=sizeof(operators)/sizeof(operators[0])-1; i>=0; i--)
		{
			if(strncmp(operators[i], str_seq, strlen(operators[i]))==0)
				return strlen(operators[i]);
		}
		return 0;
	}

	int FetchSpecialSymbol(const char *str_seq)
	{
		static const char symbols[]={'(', ')', '[', ']', '{', '}', ',', ';'};

		for(int i=0; i<sizeof(symbols)/sizeof(symbols[0]); i++)
			if(str_seq[0]==symbols[i])
				return 1;
		return 0;
	}

	int FetchSeperator(const char *str_seq)
	{
		return IsSeperator(str_seq[0]);
	}

	int FetchNumber(const char *str_seq)
	{
		int length=0;
		bool hasPoint=false;
		for(; str_seq[length]; length++)
		{
			if(str_seq[length]=='.')
			{
				if(!hasPoint)
					hasPoint=true;
				else
					break;
				continue;
			}
			if(!isdigit(str_seq[length]))
				break;
		}
		if(str_seq[length]=='f') length++;
		if(IsSeperator(str_seq[length])&&str_seq[length-1]!='.')
			return length;
		else
			return 0;
	}

	int FetchChar(const char *str_seq)
	{
		if(str_seq[0]=='\'')
		{
			if(str_seq[1]!='\\')
				return (str_seq[1]!='\''&&str_seq[2]=='\'')?3: 0;	//Error => '' no char
			else
			{
				switch(str_seq[2])
				{
					case 'n':
					case 't':
					case 'b':
					case '\\':
					case '\'':
						return str_seq[3]=='\''?4:0;
				}
			}
		}else
			return 0;
	}

	int FetchError(const char *str_seq)
	{
		int length=0;
		for(; str_seq[length]; length++)
		{
			if(IsSeperator(str_seq[length]))
				break;
		}
		return length;
	}


}