#include <iostream>
#include <map>
#include <string>
#include <list>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>

using namespace std;
typedef list<string> Rules;

map<string, Rules> grammar;
//only store non-terminal
map<string, set<string> > first;
map<string, set<string> > follow;
map<string, map<string, string> > lltable;

inline bool is_terminal(map<string, Rules>::iterator &it, const string &tok)
{
	return (it=grammar.find(tok))==grammar.end();
}

inline bool is_terminal(const string &tok)
{
	map<string, Rules>::iterator it;
	return is_terminal(it, tok);
}

set<string> get_first(string tok)
{
	if(!is_terminal(tok))
		return first[tok];
	else
		return {tok};
		
}

void load_grammar(istream &in)
{
	string line, target;

	if(!in)
		return;

	while(getline(in, line))
	{
        // Remove \r
		if(!line.empty() && *line.rbegin() == '\r')
			line.erase( line.length()-1, 1);
		if(line[0]=='\t')
		{
			grammar[target].push_back(line.substr(1));
		}
		else
			target=line;
	}
}

bool list_follow_helper(int count, const string &rule, set<string> &first_set)
{
	stringstream ss(rule);
	string token;
	auto tmp_it = grammar.begin();
	//skip n element
	for(;count;count--)
		ss>>token;
	int count2=0;
	while(ss>>token)
	{
		count2++;
		if(is_terminal(tmp_it, token))
		{
			if(count2>1)
				first_set.erase("epsilon");
			first_set.insert({token});
			break;
		}else
		{
			const auto &token_first=first[token];

			first_set.erase("epsilon");
			first_set.insert(token_first.cbegin(), token_first.cend());
			if(token_first.find("epsilon")==token_first.cend())
				break;
		}
	}

	return ss.eof();
}

// Use iteration algorithm
void list_first()
{
	first.clear();
	bool hasUpdated;
	do{
		hasUpdated=false;
		for(auto g_it=grammar.cbegin(); g_it!=grammar.cend(); g_it++)
		{
			auto &first_set=first[g_it->first];
			const auto old_first_set=first_set;

			for(const string &rule: g_it->second)
			{
				set<string> result;
				list_follow_helper(0, rule, result);
				if(result.size()>0)
					first_set.insert(result.cbegin(), result.cend());
			}
			if(old_first_set!=first_set)
				hasUpdated=true;
		}
	}while(hasUpdated);
}

void list_follow(const string &startSymbol)
{
	follow.clear();
	follow[startSymbol].insert("$");

	bool hasUpdated;
	do
	{
		hasUpdated=false;
		for(auto g_it=grammar.cbegin(); g_it!=grammar.cend(); g_it++)
		{
			auto &follow_set_A=follow[g_it->first];
			for(const string &rule: g_it->second)
			{
				stringstream ss(rule);
				string token;
				int count=0;

				while(ss>>token)
				{
					count++;

					if(!is_terminal(token))
						{
						auto &token_follow=follow[token];
						auto old_token_follow=token_follow;
						//Apply rule 2 and 3 in the text
						//pass the following string sequence to the helper
						set<string> result;
						if(list_follow_helper(count, rule, result))
						{
							if(ss.eof()||result.find("epsilon")!=result.cend())
								token_follow.insert(follow_set_A.cbegin(), follow_set_A.cend());
						}
						result.erase("epsilon");
						token_follow.insert(result.cbegin(), result.cend());

						if(old_token_follow!=token_follow)
							hasUpdated=true;
						
					}
				}

			}

		}
	}while(hasUpdated);
}

void print_set(const set<string> &s)
{
	for(auto &str: s)
	{
		cout<<str<<" ";
	}
	cout<<endl;
}

// generator for lltable[Nonterminal][Terminal], a sparse matrix
void gen_table()
{
	for(auto g_it=grammar.cbegin(); g_it!=grammar.cend(); g_it++)
	{
		for(const string &rule: g_it->second)
		{
			// A --> X1 X2 X3
			// We need First(X1) First(X2) First(X3)....
			stringstream ss(rule);
			string token;
			int count=0;
			bool hasEpsilon;
			while(ss>>token)
			{
				count++;
				hasEpsilon=false;
				const set<string> &first_set=get_first(token);
				for(const string &ter: first_set)
				{
					if(ter!="epsilon")
						lltable[g_it->first][ter]=rule;
					else
						hasEpsilon=true;
				}
				if(!hasEpsilon) break;
			}
			const set<string> &non_ter_first=first[g_it->first];
			if(non_ter_first.find("epsilon")!=non_ter_first.cend())
			{
				// if the first set of non terminal has eplison
				auto &follow_set=follow[g_it->first];
				for(const string &fol: follow_set)
					if(non_ter_first.find(fol)==non_ter_first.cend())
						lltable[g_it->first][fol]="epsilon";
			}
		}
	}
}

void dump_lltable(ostream &out)
{
	for(auto non_ter=lltable.cbegin(); non_ter!=lltable.cend(); ++non_ter)
	{
		auto &ter=non_ter->second;
		for(auto it=ter.cbegin(); it!=ter.cend(); ++it)
		{
			out<<non_ter->first<<"\t"<<it->first<<"\t"<<it->second<<endl;
		}
		out<<endl;
	}
}

void dump_non_terminal_info(map<string, set<string> > &s, ostream &out)
{
	for(auto g_it=grammar.cbegin(); g_it!=grammar.cend(); g_it++)
	{
		auto &ss=s[g_it->first];
		out<<g_it->first<<"\t";
		for(auto &str: ss)
		{
			out<<str<<" ";
		}
		out<<endl;
	}
}

int main(int argc, char *argv[])
{

	// lltable grammar start_symbol outfile
	if(argc!=4)
	{
		fprintf(stderr, "Usage: lltable grammar start_symbol outfile\n");
		return 1;
	}
	ifstream ifs(argv[1]);
	load_grammar(ifs);

	list_first();
	list_follow(argv[2]);

	ofstream file("set.txt");
	file<<"First"<<endl;
	dump_non_terminal_info(first, file);
	file<<endl<<"Follow"<<endl;
	dump_non_terminal_info(follow, file);

	gen_table();

	ofstream ofs(argv[3]);
	ofs<<argv[2]<<endl;
	dump_lltable(ofs);

	return 0;
}
