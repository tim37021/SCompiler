

cd Lexer
rm *.o
rm *.a
make
cd ..
cd Parser
make
cd ..
cd LLGen
g++ -o sc -std=c++0x *.cpp -g
