#include "lexer.h"
#include "rule.h"

#include <iostream>
#include <stdexcept>
namespace SLexer
{

	Category::Category(const char *name_, CHECKFUNC fptr):
		name(name_), check(fptr)
	{

	}

	Token::Token(const Category *cate_, const char *value_):
		category(cate_), value(value_)
	{

	}

	Token::Token(const Category *cate_, const std::string &value_):
		category(cate_), value(value_)
	{

	}

	Lexer::Lexer(const char *code_, const PrecedenceList &prec):
		code(code_), precedence(prec)
	{
		reset();
		op_pred["="]=3;
		//unary
		op_pred["+u"]=18;
		op_pred["-u"]=18;
		op_pred["+"]=13;
		op_pred["-"]=13;
		op_pred["*"]=14;
		op_pred["/"]=14;
		op_pred[">"]=12;
		op_pred[">="]=12;
		op_pred["<"]=12;
		op_pred["<="]=12;
		op_pred["=="]=11;
		op_pred["!="]=11;
		op_pred["&&"]=10;
		op_pred["||"]=10;
	}

	void Lexer::reset()
	{
		p=0;
	}

	bool Lexer::hasNext() const
	{
		return p<code.length();
	}

	Token Lexer::nextToken()
	{
		int length;
		for(int i=0; i<precedence.size(); i++)
			if((length=precedence[i]->check(code.c_str()+p))>0)
			{
				p+=length;
				return Token(precedence[i], code.substr(p-length, length));
			}
	}

	void Lexer::setPrecedence(const std::vector<const Category *> &prec)
	{
		precedence=prec;
	}

	void Lexer::encodeExpr()
	{
		int _p=p;
		std::vector<Token> tokens;
		while(hasNext())
		{
			const Token &t=nextToken();
			// we may catch right side of ) or ]
			if(t.category==Error||t.value==";"||t.value=="]"||t.value==")"||t.value==",")
				break;

			if(t.value=="(")
			{
				int balance=1;
				if(tokens.size()==0||tokens.back().category!=Identifier)
				{
					Token tt;
					tt.value=t.value;
					tt.category=Identifier;
					tokens.push_back(tt);	// Not a function call
				}
				else
					tokens.back().value+=t.value;	// append to back
				// Fetch whole function....
				while(balance&&hasNext())
				{
					const Token &t2=nextToken();
					if(t2.category==Error)
						break;
					if(t2.value=="(")
						balance++;
					if(t2.value==")")
						balance--;
					tokens.back().value+=t2.value;
				}
			} else if(t.value=="["&&tokens.size()>0&&tokens.back().category==Identifier)
			{
				int balance=1;
				tokens.back().value+=t.value;
				// Fetch whole function....
				while(balance&&hasNext())
				{
					const Token &t2=nextToken();
					if(t2.category==Error)
						break;
					if(t2.value=="[")
						balance++;
					if(t2.value=="]")
						balance--;
					tokens.back().value+=t2.value;
				}
			} else if(t.category==Number||t.category==Operator||t.category==Identifier||t.category==SpecialSymbol)
				tokens.push_back(t);
		}

		const std::string &str=encodeExpr(tokens);
		//
		// replace string
		if(str!=""&&tokens.size()>1)
		{
			code = code.substr(0, _p)+str.substr(1, str.length()-2)+code.substr(p-1);
		}
		// recover
		p=_p;
	}

	std::string Lexer::encodeExpr(const std::vector<Token> &tokens)
	{
		// infix to postfix
		try
		{
			const std::vector<Token> &postfix=infixToPostfix(tokens);
			return postfixToInfix(postfix);
		}catch(...){
			return "";
		}
		//return result;
	}

	bool Lexer::check(const std::vector<Token> &opstack, const std::string &token)
	{
		return op_pred[opstack.back().value]>=op_pred[token];
	}

	std::vector<Token> Lexer::infixToPostfix(const std::vector<Token> &tokens)
	{
		std::vector<Token> result;
		std::vector<Token> opstack;
		Token lastToken;

		for(int i=0; i<tokens.size(); i++)
		{
			auto token=tokens[i];
		
			if(token.category==Identifier||token.category==Number)
				result.push_back(token);
			else
			{
				// Error, let parser detect them...
				if(token.value=="[" || token.value=="(")
					throw std::exception();
					
				if(lastToken.value==""||lastToken.category==Operator)
					token.value=token.value+"u";

				// don't worry about ( )
				while(opstack.size()>0&&check(opstack, token.value))
				{
					result.push_back(opstack.back());
					opstack.pop_back();
				}

				opstack.push_back(token);
			}
			lastToken=token;
		}
		while(opstack.size())
		{
			result.push_back(opstack.back());
			opstack.pop_back();
		}
		return result;
	}

	std::string Lexer::postfixToInfix(const std::vector<Token> &tokens)
	{
		std::vector<std::string > result;

		for(int i=0; i<tokens.size(); i++)
		{
			auto &token=tokens[i];
			if(token.category==Identifier||token.category==Number)
				result.push_back(token.value);
			else
			{
				if(token.value=="["||token.value=="(")
					throw std::exception();
				
				std::string newone;
				// check if it is unary operator
				std::string rhs = result.back(); result.pop_back();
				if(token.value.substr(token.value.length()-1)=="u")
					newone = "("+token.value.substr(0, token.value.length()-1)+rhs+")";
				else{
					
					std::string lhs = result.back(); result.pop_back();
					newone = "("+lhs+token.value+rhs+")";
				}
				result.push_back(newone);
			}
		}

		return result.back();
	}
}