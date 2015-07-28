LIB_NAME=libSLexer.a

HW2: slexer $(LIB_NAME)

slexer: main.o lexer.o rule.o
	g++ -o slexer main.o lexer.o rule.o

$(LIB_NAME): lexer.o rule.o
	ar rcs $(LIB_NAME) lexer.o rule.o
    
main.o: main.cpp lexer.h rule.h
	g++ -g -c -std=c++0x main.cpp

lexer.o: lexer.h lexer.cpp rule.h
	g++ -g -c -std=c++0x lexer.cpp
    
rule.o: rule.h rule.cpp
	g++ -g -c -std=c++0x rule.cpp
