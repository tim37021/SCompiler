cd Lexer
mingw32-make
cd ..
cd Parser
mingw32-make
cd ..
cd LLGen
g++ -std=c++0x -g *.cpp