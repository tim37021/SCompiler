#ifndef __LLGEN_SYMBOL_H_
#define __LLGEN_SYMBOL_H_


#include <string>
#include <list>

namespace SLLGen
{
	struct Symbol
	{
		std::string type, ctype;
		std::string name;
		std::string uniqueName;
		int arraySize;
		std::list<std::string> paramTypes;

		Symbol(): arraySize(0){}

		bool isArray() const 
		{ return arraySize>0; }

		bool isFunction() const
		{ return paramTypes.size()>0; }
	};
}

#endif