#include "llgen.h"
#include <cstdlib>

#define TRUE_STEP 1
#define FALSE_STEP 0

using namespace std;

static const char *typeLLVMMap[]={"", "i8", "i32", "float", "double", "", "i8*", "i32*", "float*", "double*"};
// for variable symbol
static const char *typeLLVMMapPtr[]={"", "i8*", "i32*", "float*", "double*", "", "i8**", "i32**", "float**", "double**"};
static const char *typeCTypeMap[]={"void", "char", "int", "float", "double", "void*", "char*", "int*", "float*", "double*"};
static const char *opCMap[]={"+", "-", "*", "/", "==", "!=", ">", ">=", "<", "<=", "&&", "||"};

static string ReplaceString(string subject, const string& search,
                          const string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

namespace SLLGen
{
	LLGen::LLGen(): m_tempCounter(0)
	{
		m_ll = "@.str = private unnamed_addr constant [4 x i8] c\"%f\\0A\\00\", align 1\n";
		m_ll += "declare i32 @printf(i8*, ...) #1\n";
	}

	void LLGen::beginFunction(Type rntType, const string &name, const list<pair<Type, string> > &params)
	{
		// function must be declared in global scope
		if(m_table.isInGlobal())
		{
			m_currentFuncSymbol.paramTypes.clear();
			m_currentFuncSymbol.type=typeLLVMMap[rntType];
			m_currentFuncSymbol.ctype="function";
			m_currentFuncSymbol.name=name;

			string args;
			// used to specify it is function type
			m_currentFuncSymbol.paramTypes.push_back("DUMMY");
			for(auto &p: params)
				m_currentFuncSymbol.paramTypes.push_back(typeLLVMMap[p.first]);
			m_table.insert(name, m_currentFuncSymbol);

			m_table.push();

			for(auto &p: params)
			{
				addVariableDecl(p.first, p.second, 0, true);
				string uniqueName;
				m_table.getUniqueName(p.second, uniqueName);
				args=args+typeLLVMMap[p.first]+" "+uniqueName+", ";
			}

			args = args.size()>0? args.substr(0, args.size()-2): args;
			m_funcLL="define "+m_currentFuncSymbol.type+" @"+m_currentFuncSymbol.name+"("+args+") #0 {\n";

			// prevent redeclaration problem
			m_table.push();

			m_tempCounter=0;
			m_recentData.clear();
			m_addressedParameter.clear();
			m_ifClause.clear();
			m_endBlock.clear();
			m_endBlock.push_back(false);
			m_currentTargetLL=&m_funcLL;

			if(m_currentFuncSymbol.type!="void")
			{
				// always %1 :)
				auto rntVar=allocTemp(typeLLVMMapPtr[rntType]);
				addVariableDecl(rntType, rntVar.substr(1), 0, false, true);
			}

			for(auto &p: params)
			{
				// This one is tricky
				auto tempVar=allocTemp(typeLLVMMapPtr[p.first]);

				string uniqueName;
				m_table.getUniqueName(p.second, uniqueName);
				m_addressedParameter[uniqueName]=tempVar;

				addVariableDecl(p.first, tempVar.substr(1), 0, false, true);
				m_funcLL=m_funcLL+"store volatile "+typeLLVMMap[p.first]+" "+uniqueName+", "+
					typeLLVMMapPtr[p.first]+" "+tempVar+"\n";
			}
		}else
			throw InvalidOperation();
	}

	void LLGen::endFunction()
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: endfunction must be placed in function");
		string varName, varType;
		prepare("%1", varName, varType);
		string exitLabel;
		if(m_endBlock.back()) exitLabel=allocTemp("label");
		auto tempVar=allocTemp(ref(varType));
		m_funcLL+=tempVar+" = load volatile "+varType+" %1, align 4\n";
		m_funcLL+="ret "+ref(varType)+" "+tempVar+"\n";
		m_funcLL+="}\n";
		m_table.pop();
		m_table.pop();

		m_ll+=ReplaceString(m_funcLL, "{EXIT_HERE}", exitLabel);
	}

	void LLGen::addVariableDecl(Type t, const std::string &name, int arraySize, bool is_arg, bool no_insert)
	{
		// Make our symbol first
		Symbol var;
		var.type=string(typeLLVMMap[t])+(!is_arg?"*":"");
		var.ctype=string(typeCTypeMap[t])+(arraySize?"[]": "");
		var.name=name;
		var.arraySize=arraySize;

		string type=typeLLVMMap[t];
		string align="4";
		if(type=="double")
			align="8";
		if(var.isArray())
		{
			var.type=(type="["+to_string(var.arraySize)+" x "+type+"]")+"*";
			align="16";
		}

		if(!no_insert)
			m_table.insert(name, var);
		// There's no need to generate code when var decl is argument
		if(is_arg) return;
		string uniqueName;
		m_table.getUniqueName(name, uniqueName);



		if(m_table.isInGlobal())
		{
			string initializer=(var.isArray()? "zeroinitializer": "0");
			m_ll+=uniqueName+" = global "+type+" "+initializer+", align "+align+"\n";
		}else
			*m_currentTargetLL+=uniqueName+" = alloca "+type+", align "+align+"\n";
	}

	void LLGen::addFunctionDecl(Type rntType, const string &name, const list<pair<Type, string> > &params)
	{
		Symbol funcSymbol;
		
		funcSymbol.paramTypes.clear();
		funcSymbol.type=typeLLVMMap[rntType];
		funcSymbol.name=name;

		string args;
		// used to specify it is function type
		funcSymbol.paramTypes.push_back("DUMMY");
		for(auto &p: params)
			funcSymbol.paramTypes.push_back(typeLLVMMap[p.first]);
		m_table.insert(name, funcSymbol);
	}

	string LLGen::addBinaryExpr(const std::string &lhs, BinaryOperation op, const std::string &rhs)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation();

		string lhs_type;
		string rhs_type;
		string lhsUnique;
		string rhsUnique;

		prepare(lhs, lhsUnique, lhs_type);
		prepare(rhs, rhsUnique, rhs_type);

		if(lhs_type!=rhs_type)
		{
			string targetType;
			if(isPointerType(lhs_type)||isPointerType(rhs_type))
				throw TypeError("TypeError: Not allow pointer type conversion "+lhs_type+" vs "+rhs_type);
			// try implicit conversion
			if(lhs_type=="double"||rhs_type=="double")
				targetType="double";
			else if(lhs_type=="float"||rhs_type=="float")
				targetType="float";
			else if(lhs_type=="i32"||rhs_type=="i32")
				targetType="i32";
			else if(lhs_type=="i8"||rhs_type=="i8")
				targetType="i8";
			else if(lhs_type=="i1"||rhs_type=="i1")
			    targetType="i1";
			else
				throw TypeError("TypeError: Unknown type conversion "+lhs_type+" vs "+rhs_type);

            m_warningLog+="Type of lhs("+lhs_type+") is different from type of rhs("+rhs_type+") op='"+opCMap[op]+"', convert to "+targetType+"\n";

			lhsUnique=castType(lhs_type, lhsUnique, targetType);
			rhsUnique=castType(rhs_type, rhsUnique, targetType);
			lhs_type=rhs_type=targetType;
		}

		if(!isSymbol(lhsUnique)&&!isSymbol(rhsUnique)&&lhs_type=="i32")
		{
			int lhsValue=atoi(lhsUnique.c_str());
			int rhsValue=atoi(rhsUnique.c_str());
			switch(op)
			{
				case BO_ADD:
					return to_string(lhsValue+rhsValue);
				case BO_SUB:
					return to_string(lhsValue-rhsValue);
				case BO_MUL:
					return to_string(lhsValue*rhsValue);
				case BO_DIV:
					return to_string(lhsValue/rhsValue);
				case BO_EQ:
					return to_string(lhsValue==rhsValue);
				case BO_NE:
					return to_string(lhsValue!=rhsValue);
				case BO_GT:
					return to_string(lhsValue>rhsValue);
				case BO_GE:
					return to_string(lhsValue>=rhsValue);
				case BO_LT:
					return to_string(lhsValue<rhsValue);
				case BO_LE:
					return to_string(lhsValue<=rhsValue);
			}
		}
		if(lhs_type=="i32"||lhs_type=="i8"||lhs_type=="i1")
		{
			string outType;
			string code=genBasicOperation_i32(op, lhsUnique, rhsUnique, lhs_type, outType);
			string tmpVar=allocTemp(outType);
			*m_currentTargetLL+=tmpVar+" = "+code+"\n";
			return tmpVar;
		}
		if(lhs_type=="float"||lhs_type=="double")
		{
			string outType=lhs_type;
			string code=genBasicOperation_floating(op, lhsUnique, rhsUnique, lhs_type, outType);
			string tmpVar=allocTemp(outType);
			*m_currentTargetLL+=tmpVar+" = "+code+"\n";
			return tmpVar;
		}

		throw InvalidOperation("BinaryOperation");
	}

	string LLGen::addUnaryExpr(UnaryOperation op, const string &rhs)
	{
		string rhsUnique, rhs_type;
		prepare(rhs, rhsUnique, rhs_type);
		if(rhs_type=="i32"&&!isSymbol(rhsUnique))
		{
			int rhsValue=atoi(rhsUnique.c_str());
			switch(op)
			{
				case UO_MINUS:
					return to_string(-rhsValue);
				case UO_NOT:
					return to_string(!rhsValue);
			}
		}
		
	    if(op==UO_MINUS)
	        return addBinaryExpr("0", BO_SUB, rhsUnique);
	    else
	        return castType(rhs_type, rhsUnique, "i1");
	}

	string LLGen::addAssignStmt(const string &lhs, const string &rhs)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation();

		string lhs_type;
		string lhsUnique;
		// don't load!!!!!
		prepare(lhs, lhsUnique, lhs_type, true);

		string rhs_type;
		string rhsUnique;
		prepare(rhs, rhsUnique, rhs_type);

		// if lhs type is pointer type of rhs
		// and lhs must be lvalue
		if(!isSymbol(lhsUnique))
			throw InvalidOperation();

		if(isPointerType(lhs_type)&&m_addressedParameter.find(lhsUnique)==m_addressedParameter.cend())
		{
			string targetType=ref(lhs_type);
			if(rhs_type!=targetType)
			    m_warningLog+="Type of rhs("+rhs_type+") does not match!, convert to "+targetType+"\n";
			rhsUnique=castType(rhs_type, rhsUnique, targetType);
			rhs_type=targetType;
		}else if(!isTempVar(lhs_type))
		{
			string targetType=lhs_type;
			if(rhs_type!=targetType)
			    m_warningLog+="Type of rhs("+rhs_type+") does not match!, convert to "+targetType+"\n";
			rhsUnique=castType(rhs_type, rhsUnique, targetType);
			rhs_type=targetType;
		}

		if(lhs_type==rhs_type+"*"&&m_addressedParameter.find(lhsUnique)==m_addressedParameter.cend())
		{
			*m_currentTargetLL+="store volatile "+rhs_type+" "+rhsUnique+", "+lhs_type+" "+lhsUnique+"\n";
			// Cache it whether it is number literal !!
			m_recentData[lhsUnique]=rhsUnique;
			return lhsUnique;
		}

		if(lhs_type==rhs_type)
		{
			// This must be function parameter
			string p_addr=m_addressedParameter[lhsUnique];
			*m_currentTargetLL+="store volatile "+rhs_type+" "+rhsUnique+", "+lhs_type+"* "+p_addr+"\n";
			// Cache it whether it is number literal !!
			m_recentData[lhsUnique]=rhsUnique;
			return lhsUnique;
		}

		throw TypeError("Type Error: &"+lhs+"(aka "+lhs_type+") vs "+rhsUnique+"(aka "+rhs_type+")");
	}

	void LLGen::addReturnStmt(const string &rhs)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation();

		string rhs_type;
		string rhsUnique;
		prepare(rhs, rhsUnique, rhs_type);
		
		if(m_currentFuncSymbol.type!=rhs_type)
		{
		    m_warningLog+="Return type("+rhs_type+") not match!, convert to "+m_currentFuncSymbol.type+"\n";
		}
		string casted=castType(rhs_type, rhs, m_currentFuncSymbol.type);

		if(!m_endBlock.back())
		{
			addAssignStmt("%1", casted);
			*m_currentTargetLL+="br label {EXIT_HERE}\n";
		}
		m_endBlock.back()=true;
	}

	string LLGen::getSymbolType(const string &name)
	{
		Symbol result;
		if(m_table.lookUp(name, result))
			return result.type;
		else
			throw UndefinedSymbol();
	}

	string LLGen::genBasicOperation_i32(BinaryOperation op, const std::string &lhs, 
		const std::string &rhs, const string &type_, std::string &outType)
	{
		string head;
		string castedLhs=lhs;
		string castedRhs=rhs;
		string type=type_;
		switch(op)
		{
			case BO_ADD:
				head="add"; outType=type; break;
			case BO_SUB:
				head="sub"; outType=type; break;
			case BO_MUL:
				head="mul"; outType=type; break;
			case BO_DIV:
				head="sdiv"; outType=type; break;
			case BO_EQ:
				head="icmp eq"; outType="i1"; break;
			case BO_NE:
				head="icmp ne"; outType="i1"; break;
			case BO_GT:
				head="icmp sgt"; outType="i1"; break;
			case BO_GE:
				head="icmp sge"; outType="i1"; break;
			case BO_LT:
				head="icmp slt"; outType="i1"; break;
			case BO_LE:
				head="icmp sle"; outType="i1"; break;
			case BO_AND:
			    castedLhs=castType(type, lhs, "i1");
			    castedRhs=castType(type, rhs, "i1");
			    type="i1";
				head="and"; outType="i1"; break;
			case BO_OR:
			    castedLhs=castType(type, lhs, "i1");
			    castedRhs=castType(type, rhs, "i1");
			    type="i1";
				head="or"; outType="i1"; break;
		}

		return head+" "+type+" "+castedLhs+", "+castedRhs;
	}

	string LLGen::genBasicOperation_floating(BinaryOperation op, const string &lhs, 
		const string &rhs, const string &type, string &outType)
	{
		string head;
		switch(op)
		{
			case BO_ADD:
				head="fadd"; outType=type; break;
			case BO_SUB:
				head="fsub"; outType=type; break;
			case BO_MUL:
				head="fmul"; outType=type;  break;
			case BO_DIV:
				head="fdiv"; outType=type; break;
			case BO_EQ:
				head="fcmp oeq"; outType="i1"; break;
			case BO_NE:
				head="fcmp one"; outType="i1"; break;
			case BO_GT:
				head="fcmp ogt"; outType="i1"; break;
			case BO_GE:
				head="fcmp oge"; outType="i1"; break;
			case BO_LT:
				head="fcmp olt"; outType="i1"; break;
			case BO_LE:
				head="fcmp ole"; outType="i1"; break;
		}

		return head+" "+type+" "+lhs+", "+rhs;
	}

	bool LLGen::isPointerType(const string &type)
	{
		return type.substr(type.length()-1)=="*";
	}

	string LLGen::ref(const string &type)
	{
		if(isPointerType(type))
			return type.substr(0, type.length()-1);
		else
			throw TypeError("TypeError: Try to reference a non pointer type value."+type);
	}

	string LLGen::loadMemory(const string &name_, const string &sub)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation();

		if(!isSymbol(name_))
			return name_;

		string name=isTempVar(name_)?name_.substr(1):name_;

		Symbol result;
		if(m_table.lookUp(name, result))
		{
			string uniqueName;
			m_table.getUniqueName(name, uniqueName);
			//check type;
			if(isPointerType(result.type))
			{
				if(sub!="")
				{
					string &cachePtr=m_recentData[uniqueName+"+"+sub];
					if(result.isArray())
					{
						string elementType=getElementType(result.type);
						if(cachePtr=="")
						{
							cachePtr=allocTemp(elementType+"*");
							*m_currentTargetLL+=cachePtr+"= getelementptr inbounds "+result.type+" "+uniqueName+", i32 0, i32 "+sub+"\n";
						}
						string &tmpVarName=m_recentData[uniqueName+"["+sub+"]"];
						if(tmpVarName=="")
						{
							tmpVarName=allocTemp(elementType);
							*m_currentTargetLL+=tmpVarName+" = load volatile "+elementType+"* "+cachePtr+", align 4\n";
						}
						return tmpVarName;
					}else if(m_addressedParameter.find(uniqueName)!=m_addressedParameter.cend())
					{
						// pointer [ sub ] 
						string elementType=ref(result.type);
						if(cachePtr=="")
						{
							cachePtr=allocTemp(elementType+"*");
							*m_currentTargetLL+=cachePtr+"= getelementptr inbounds "+result.type+" "+uniqueName+", i32 "+sub+"\n";
						}
						string &tmpVarName=m_recentData[uniqueName+"["+sub+"]"];
						if(tmpVarName=="")
						{
							tmpVarName=allocTemp(elementType);
							*m_currentTargetLL+=tmpVarName+" = load volatile "+elementType+"* "+cachePtr+", align 4\n";
						}
						return tmpVarName;
					}else
						throw InvalidOperation("InvalidOperation: Unknown State");
				}else if(m_addressedParameter.find(name)==m_addressedParameter.cend() && !result.isArray())
				{
					// local variable
					// Use unique name!!!
					string &cache=m_recentData[uniqueName];
					if(cache!="") return cache;

					string tmpVarName=allocTemp(result.type.substr(0, result.type.length()-1));
					*m_currentTargetLL+=tmpVarName+" = load volatile "+result.type+" "+uniqueName+", align 4\n";
					
					cache=tmpVarName;
					return tmpVarName;
				}else
					return name;
			}else if(!isTempVar(name))
			{
				// load recent data for parameter
				string &cache=m_recentData[uniqueName];
				string target=cache;

				if(cache=="")
				{
					// Load it!!
					target=m_addressedParameter[uniqueName];

					string tmpVarName=allocTemp(result.type);
					*m_currentTargetLL+=tmpVarName+" = load volatile "+result.type+"* "+target+", align 4\n";
					target=tmpVarName;
				}
				cache=target;
				return target;
			}else
				throw TypeError("Type Error in LoadMemory"+name+"["+sub+"]");
		}else
			throw UndefinedSymbol("UndefinedSymbol: "+name+"["+sub+"]");
		
	}

	string LLGen::getElementType(const string &type)
	{
		auto left = type.find_first_of("[");
		auto right = type.find_last_of("]");
		if(isPointerType(type)&&left!=string::npos&&right!=string::npos)
		{
			auto mid=type.find_first_of("x");
			return type.substr(mid+2, right-mid-2);
		}else
			throw InvalidOperation("Type Error: "+type+" is not an array type");
	}

	string LLGen::addFunctionCall(const string &func, const list<string> &params)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation();

		Symbol func_symbol;
		if(!m_table.lookUp(func, func_symbol)||func_symbol.paramTypes.front()!="DUMMY")
			throw TypeError("Type Error: The symbol is not function");
		if(func_symbol.paramTypes.size()!=params.size()+1)
			throw TypeError("Type Error: Number of argument does not match");

		

		auto it=func_symbol.paramTypes.cbegin();
		auto it2=params.cbegin();
		++it;
		string arg_list;
		for(;it!=func_symbol.paramTypes.cend()&&it2!=params.cend(); ++it,++it2)
		{
			string pUnique, p_type;
			string castedValue;
			Symbol temp;
			m_table.lookUp(*it2, temp);
			if(!temp.isArray())
			{
				prepare(*it2, pUnique, p_type);
				// cast type to argument type
				castedValue=castType(p_type, pUnique, *it);
			}
			else
			{
				string &cachePtr=m_recentData[temp.uniqueName+"+0"];
				if(cachePtr=="")
				{
					cachePtr=allocTemp(getElementType(temp.type)+"*");
					*m_currentTargetLL+=cachePtr+"= getelementptr inbounds "+temp.type+" "+temp.uniqueName+", i32 0, i32 0\n";
				}
				castedValue=cachePtr;
			}
			
			arg_list+=*it+" "+castedValue+", ";
		}
		string tmpVar=allocTemp(func_symbol.type);
		*m_currentTargetLL+=tmpVar+" = tail call "+func_symbol.type+" @"+func+"(";
		*m_currentTargetLL+=(arg_list.length()>0?arg_list.substr(0, arg_list.length()-2):arg_list)+")\n";
		return tmpVar;
	}

	void LLGen::beginIfClause(const string &cond)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: IfClause must be placed in function");

		string uniqueName;
		string type;
		prepare(cond, uniqueName, type);

	    uniqueName = castType(type, uniqueName, "i1");

		m_table.push();
		m_ifClause.push_back({});
		m_endBlock.push_back(false);
		m_currentTargetLL=&m_ifClause.back().trueClause;
		m_ifClause.back().trueLabel = allocTemp("label");
		m_ifClause.back().cond = uniqueName;
		m_clauseStep.push_back(TRUE_STEP);
	}

	void LLGen::elseClause()
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: elseClause must be placed in function");

		if(m_clauseStep.back()==TRUE_STEP)
		{
			// force reload all variable
			for(auto &p: m_recentData)
			{
				if(!isTempVar(p.first))
					p.second="";
			}
			if(!m_endBlock.back())
				m_ifClause.back().trueClause+="br label {JUMP_END_IF}\n";
			m_endBlock.back()=false;
			m_clauseStep.back()=FALSE_STEP;
			m_currentTargetLL=&m_ifClause.back().falseClause;
			m_ifClause.back().falseLabel=allocTemp("label");
		}else
			throw InvalidOperation("InvalidOperation: else cannot be placed here");
	}

	void LLGen::endIfClause()
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: endIf must be placed in function");
		string trueCode, falseCode, cond, trueLabel, falseLabel;
		m_table.pop();

		trueCode=m_ifClause.back().trueClause;
		trueLabel=m_ifClause.back().trueLabel;
		falseCode=m_ifClause.back().falseClause;
		falseLabel=m_ifClause.back().falseLabel;

		cond=m_ifClause.back().cond;
		bool step = m_clauseStep.back();

		// pop this if statement
		m_ifClause.pop_back();
		m_clauseStep.pop_back();

		if(m_clauseStep.size()>0)
			m_currentTargetLL=m_clauseStep.back()? &m_ifClause.back().trueClause: &m_ifClause.back().falseClause;
		else
			m_currentTargetLL=&m_funcLL;

		if(step)
		{
			string ending=allocTemp("label");
			*m_currentTargetLL+="br i1 "+cond+", label "+trueLabel+", label "+ending+"\n";
			*m_currentTargetLL+=trueCode;
			if(!m_endBlock.back())
				*m_currentTargetLL+="br label "+ending+"\n";
			m_endBlock.pop_back();
		}else
		{
			string ending;
			*m_currentTargetLL+="br i1 "+cond+", label "+trueLabel+", label "+falseLabel+"\n";
			*m_currentTargetLL+=trueCode+falseCode;
			if(m_currentTargetLL->find("{JUMP_END_IF}")!=string::npos)
			{
				ending=allocTemp("label");
				*m_currentTargetLL=ReplaceString(*m_currentTargetLL, "{JUMP_END_IF}", ending);
			}
			if(!m_endBlock.back())
			{
				if(ending=="")
					ending=allocTemp("label");
				*m_currentTargetLL+="br label "+ending+"\n";
			}
		}

		// force reload all variable
		for(auto &p: m_recentData)
		{
			if(!isTempVar(p.first))
				p.second="";
		}
	}

	void LLGen::addPrintStmt(const string &rhs)
	{
		string rhsUnique, rhs_type;
		prepare(rhs, rhsUnique, rhs_type);
		string casted=castType(rhs_type, rhsUnique, "double");
		string dummy=allocTemp("i32");
		*m_currentTargetLL+=dummy+" = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i32 0, i32 0), double "+casted+")\n";
	}

	void LLGen::beginWhileCondEvalution()
	{
		m_beforeLabel=allocTemp("label");
		*m_currentTargetLL+="br label "+m_beforeLabel+"\n";
		// force reload all variable
		for(auto &p: m_recentData)
		{
			if(!isTempVar(p.first))
				p.second="";
		}
	}

	void LLGen::beginWhile(const string &cond)
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: beginWhile must be placed in function");

		beginIfClause(cond);
		m_ifClause.back().beforeLabel=m_beforeLabel;
	}

	void LLGen::endWhile()
	{

		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: endWhile must be placed in function");
		if(m_ifClause.back().beforeLabel=="")
			throw InvalidOperation("InvalidOperation: endWhile can not be placed here");

		string trueCode, falseCode, cond, trueLabel, falseLabel, beforeLabel;
		m_table.pop();


		trueCode=m_ifClause.back().trueClause;
		m_ifClause.back().beforeLabel;
		beforeLabel=m_ifClause.back().beforeLabel;
		trueLabel=m_ifClause.back().trueLabel;
		falseCode=m_ifClause.back().falseClause;
		falseLabel=m_ifClause.back().falseLabel;

		cond=m_ifClause.back().cond;
		bool step = m_clauseStep.back();

		// pop this if statement
		m_ifClause.pop_back();
		m_clauseStep.pop_back();

		if(m_clauseStep.size()>0)
			m_currentTargetLL=m_clauseStep.back()? &m_ifClause.back().trueClause: &m_ifClause.back().falseClause;
		else
			m_currentTargetLL=&m_funcLL;

		if(step)
		{
			string ending=allocTemp("label");
			*m_currentTargetLL+="br i1 "+cond+", label "+trueLabel+", label "+ending+"\n";
			*m_currentTargetLL+=ReplaceString(trueCode, "{BREAK_WHILE}", ending);
			if(!m_endBlock.back())
				*m_currentTargetLL+="br label "+beforeLabel+"\n";
			m_endBlock.pop_back();
		}else
			throw InvalidOperation("InvalidOperation: Unknown error");

		// force reload all variable
		for(auto &p: m_recentData)
		{
			if(!isTempVar(p.first))
				p.second="";
		}
	}
	
	void LLGen::addBreakStmt()
	{
		if(m_table.isInGlobal())
			throw InvalidOperation("InvalidOperation: break must be placed in function");
		bool found=false;
		for(int i=0; i<m_ifClause.size(); i++)
    		if(m_ifClause[i].beforeLabel!="")
    		    found=true;
    
        if(!found)
    		throw InvalidOperation("InvalidOperation: break can not be placed here");

        if(!m_endBlock.back())
		{
		    allocTemp("label");
			*m_currentTargetLL+="br label {BREAK_WHILE}\n";
		}
	}

	void LLGen::prepare(const string &name_, string &outNameValue,
		string &outType, bool dontLoad)
	{
		size_t niddle=name_.find_first_of("[");
		string name=name_;
		string sub;
		if(niddle!=string::npos)
		{
			name=name_.substr(0, niddle);
			string tmp=name_.substr(niddle+1, name_.length()-niddle-2);
			string sub_type;
			prepare(tmp, sub, sub_type);
			if(sub_type!="i32")
				throw TypeError("Type Error: Only i32 can be used for array subscripting");
		}

		if(isSymbol(name))
		{
			bool isTemp=isTempVar(name);
			string tmp=isTemp?name.substr(1):name;
			m_table.getUniqueName(tmp, outNameValue);
			Symbol outVar;
			m_table.lookUp(tmp, outVar);
			outType=outVar.type;

			// Check if this variable has been loaded(clang)
			Symbol dummy;
			if(!dontLoad&&!isTemp)
			{
				//if(sub!="")
				//{
				//	outNameValue=loadMemory(name, sub);
				//	outType=getElementType(outType);
				//}else
				//{
					outNameValue=loadMemory(name);
					if(isPointerType(outType))
						outType=ref(outType);
				//}
			}
			if(m_addressedParameter.find(outNameValue)!=m_addressedParameter.cend()&&isPointerType(outType))
			{
				if(dontLoad)
				{
					string &cachePtr=m_recentData[outNameValue+"+"+sub];
					if(cachePtr=="")
					{
						cachePtr=allocTemp(outType);
						*m_currentTargetLL+=cachePtr+"= getelementptr inbounds "+outType+" "+outNameValue+", i32 "+sub+"\n";
					}
					outNameValue=cachePtr;
				}else
				{
					if(!isPointerType(ref(outType)))
					{
						// treat as normal variable
						outNameValue=loadMemory(name);
						if(isPointerType(outType))
							outType=ref(outType);
					}else
						throw InvalidOperation("Something went wrong");
				}
			}
			if(dontLoad&&!isTemp&&outVar.isArray())
			{
				string &cachePtr=m_recentData[outNameValue+"+"+sub];
				outType=getElementType(outVar.type)+"*";
				if(cachePtr=="")
				{
					cachePtr=allocTemp(outType);
					*m_currentTargetLL+=cachePtr+"= getelementptr inbounds "+outVar.type+" "+outNameValue+", i32 0, i32 "+sub+"\n";
				}

				outNameValue=cachePtr;
			}
		}else
		{
			outNameValue=name;
			if(outNameValue.find(".")==string::npos)
				outType="i32";
			else
				outType="double";
		}
	}

	string LLGen::castType(const string &type, const string &var, const string &targetType)
	{
		if(type==targetType)
			return var;

		string casted;
		if(isSymbol(var))
			casted=allocTemp(targetType);
		else
		    casted=var;
		if(targetType=="i1")
		{
		    if(type=="i8"||type=="i32")
		    {
		        if(isSymbol(var))
                    *m_currentTargetLL+=casted+" = icmp ne "+type+" "+var+", 0\n";
                else
                    return to_string(atoi(var.c_str())!=0);
		    }else
		        throw InvalidOperation("InvalidOperation: Cannot convert floating to boolean.");
		} else if(type=="i1"||type=="i8"||type=="i32")
		{
			if(isSymbol(var)&&targetType[0]=='i')
			{
			    int src=atoi(type.c_str()+1);
			    int tar=atoi(targetType.c_str()+1);
			    if(src>tar)
			        *m_currentTargetLL+=casted+" = trunc "+type+" "+var+" to "+targetType+"\n";
			    else
			        *m_currentTargetLL+=casted+" = zext "+type+" "+var+" to "+targetType+"\n";
			}
			else if(isSymbol(var)&&(targetType=="float" || targetType=="double"))
				*m_currentTargetLL+=casted+" = sitofp "+type+" "+var+" to "+targetType+"\n";
			else if(targetType=="float" || targetType=="double")
				casted=var+".0";
			else
				casted=var;
		}else if(type=="float")
		{
			if(targetType=="double")
			{
				if(isSymbol(var))
					*m_currentTargetLL+=casted+" = fpext "+type+" "+var+" to double\n";
				else
					casted=var;
			}else
			{
				if(isSymbol(var))
					*m_currentTargetLL+=casted+" = fptosi float "+var+" to "+targetType+"\n";
				else
					casted=var.substr(0, var.find("."));
			}
		} else if(type=="double")
		{
			if(targetType=="float")
			{
			    if(!isSymbol(var))
			        casted=allocTemp(targetType);
				*m_currentTargetLL+=casted+" = fptrunc "+type+" "+var+" to float\n";
			}else
			{
				if(isSymbol(var))
					*m_currentTargetLL+=casted+" = fptosi double "+var+" to "+targetType+"\n";
				else
					casted=var.substr(0, var.find("."));
			}
		} else
			throw TypeError("TypeError: Unable to cast from "+type+" to "+targetType);
		return casted;
	}

	int LLGen::isTempVar(const string &name)
	{
		if(name.length()>=2&&name[0]=='%'&&isdigit(name[1]))
			return atoi(name.c_str()+1);
		else
			return 0;
	}

	string LLGen::allocTemp(const std::string &type)
	{
		Symbol symbol;
		symbol.type=type;
		symbol.name=to_string(++m_tempCounter);

		m_table.insert(symbol.name, symbol);
		// We are pretty sure temp name will not collide, no need to use getUniqueName
		return "%"+symbol.name;
	}

	bool LLGen::isSymbol(const string &name) const
	{
		return !(isdigit(name[0])||(name[0]=='-'&&isdigit(name[1])));
	}
	
	string LLGen::getWarningLog() const
	{
	    string tmp=m_warningLog;
	    ReplaceString(tmp, "i32", "int");
	    ReplaceString(tmp, "i8", "char");
	    ReplaceString(tmp, "i1", "bool");
	    return tmp;
	}
}