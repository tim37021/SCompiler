#ifndef _HW1_LEXER_H_
#define _HW1_LEXER_H_

#include <string>
#include <vector>
#include <map>

typedef int (*CHECKFUNC)(const char *);

namespace SLexer
{
	struct Category
	{
		std::string name;
		CHECKFUNC check;

		Category(const char *, CHECKFUNC);
	};

	typedef std::vector<const Category *> PrecedenceList;

	struct Token
	{
		const Category *category;
		std::string value;
		Token()=default;
		Token(const Category *, const char *);
		Token(const Category *, const std::string &);
	};

	class Lexer
	{
	public:
		Lexer(const char *code, const PrecedenceList &prec);
		Token nextToken();
		void reset();
		bool hasNext() const;
		// parenthesize following expression for llparser to achieve left associativity
		void encodeExpr();
		std::string getCode() const { return code; }

		void setPrecedence(const std::vector<const Category *> &);
	private:
		std::string encodeExpr(const std::vector<Token> &);
		std::vector<Token> infixToPostfix(const std::vector<Token> &);
		std::string postfixToInfix(const std::vector<Token> &);
		bool check(const std::vector<Token> &, const std::string &);

		std::map<std::string, int> op_pred;
		std::string code;
		int p;
		std::vector<const Category *> precedence;
	};

}

#endif