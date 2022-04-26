#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <cstdio>
#include <stack>
#include <numeric>

#define debug cout

using namespace std;

vector< vector<char> > G;//存放文法，一个vector二维数组，一行存放一行文法，具体的vector嵌套用法可以去搜索一下“C++ 中vector的嵌套使用”这篇博客
map< char , set<char> > terminal;//存放终结符到其FIRST集合的映射
map< char , set<char> > nonterminal;//存放非终结符到其FIRST集合的映射

vector<string> token;//读入词法分析输出的token，并将其转换成我写的二型文法所能识别的符号串，即终结符（包括number，identifer，type，keyword，界符，运算符）

void read_grammar2() {
	ifstream read("grammar2.txt");
	string content;
	vector<char> temp;//存放每一行除了推导符以外的字符，并读入文法G
	set<char> t;//用于给map增加结点（非终结符和终结符）
	while (getline(read, content)) {
		if (content[0] == '#') {
			if (content == "#endread") break;
			else continue;
		}
		nonterminal[content[0]] = t;
		//接下来开始读入二型文法
		for (int i = 0; i < content.size(); i++) {
			char ch = content[i];//ch存放当前的content的第i个字符
			if (ch != '-' && ch != '>') {
				temp.push_back(ch);
			}
			if (i >= 3 && (content[i] < 'A' || content[i]>'Z')) terminal[content[i]] = t;
		}
		G.push_back(temp);
		temp.clear();
	}
	//拓广文法,S'->S，这里方便表示将S'用Z表示
	temp.push_back('Z');
	nonterminal['Z'] = t;
	temp.push_back(G[0][0]);
	G.insert(G.begin(), temp);
}

//查看读入的二型文法去处注释后的拓广文法
void show_grammar2() {
	for (int i = 0; i < G.size(); i++) {
		for (int j = 0; j < G[i].size(); j++) {
			debug << G[i][j];
			if (j == 0) debug << "->";
			if (j == G[i].size() - 1) debug << endl;
		}
	}
}

//对词法分析的token串进行处理并存入token的vector数组
void deal_with_token() {
	ifstream read("token_output.txt");
	string content;
	while (getline(read, content)) {
		size_t flag1 , flag2 ;//flag1和flag2分别对应token三元组第一个逗号和最后一个逗号的位置
		for (flag1 = 0; flag1 < content.size(); flag1++) {
			if (content[flag1] == ',') break;
		}
		for (flag2 = content.size(); flag2 > flag1; flag2--) {
			if (content[flag2] == ',') break;
		}
		string str1, str2;
		for (int i = flag1 + 1; i < flag2; i++) {
			str1 += content[i];
		}
		for (int i = flag2 + 1; i < content.size(); i++) {
			str2 += content[i];
		}
		if (str1 == "return") {
			token.push_back("r");
			continue;
		}
		if (str1 == "int" || str1 == "void" || str1 == "bool" || str1 == "float" || str1 == "double" || str1 == "char") {
			token.push_back("t");
			continue;
		}
		if (str1 == "if") {
			token.push_back("j");
			continue;
		}
		if (str1 == "else") {
			token.push_back("k");
			continue;
		}
		if (str1 == "while") {
			token.push_back("w");
			continue;
		}
		if (str1 == "break") {
			token.push_back("b");
			continue;
		}
		if (str1 == "continue") {
			token.push_back("c");
			continue;
		}
		if (str2 == "limiter") {
			token.push_back(str1);
			continue;
		}
		if (str2 == "identifier" || str1=="main") {
			token.push_back("i");
			continue;
		}
		if (str2 == "number") {
			token.push_back("n");
			continue;
		}
		if (str2 == "operator") {
			if (str1 == ">" || str1 == "<" || str1 == ">=" || str1 == "<=" || str1 == "==" || str1 == "!=") {
				token.push_back("p");
				continue;
			}
			else {
				if (str1 == "=") token.push_back("=");
				else token.push_back("o=");
				continue;
			}
		}
	}
}

//查看token转换后的符号串序列
void show_token() {
	string temp;
	for (int i = 0; i < token.size(); i++) {
		temp += token[i];
	}
	debug << temp;
}

//根据二型文法得到各个符号的First集合
void get_First() {
	for (auto &it : terminal) {
		it.second.insert(it.first);//终结符的first是他本身
	}
	bool ischanged = true;
	while (ischanged) {
		ischanged = false;
		for (auto& it : nonterminal) {
			//对于每个非终结符遍历一次所有文法
			for (int i = 0; i < G.size(); i++) {
				//G是一个二维数组，行表示每行文法，列表示每行文法的每个字符
				int size = it.second.size();
				if (G[i][0] == it.first) {
					//遍历到了it迭代器指向的那个nonterminal
					auto iter = terminal.find(G[i][1]);//iter迭代器去终结符的map中寻找，找到则返回对应地址，否则返回terminal.end()
					if (iter != terminal.end() || G[i][1] == '@') {
						//A->a直接是一个终结符的情况 || A->@推向空的情况
						it.second.insert(G[i][1]);
						if (it.second.size() > size) ischanged = true;
					}
					else {
						//A->BC的情况下，把FIRST(B)加入FIRST(A)，若B能推出空，则把FIRST(C)加入FIRST(A)
						bool can_to_empty = false;
						for (auto j = nonterminal[G[i][1]].begin(); j != nonterminal[G[i][1]].end(); j++) {
							if (*j == '@') can_to_empty = true;
						}
						if (!can_to_empty) {
							//B不能推出空，将FIRST(B)加入FIRST(A)
							for (auto j = nonterminal[G[i][1]].begin(); j != nonterminal[G[i][1]].end(); j++) {
								it.second.insert(*j);
								if (it.second.size() > size) ischanged = true;
							}
						}
						else {
							//B能推出空，将{FIRST(B)-{@}}并上{FIRST(C)}加入FIRST(A)
							for (auto j = nonterminal[G[i][1]].begin(); j != nonterminal[G[i][1]].end(); j++) {
								if (*j != '@') it.second.insert(*j);
								if (it.second.size() > size) ischanged = true;
							}
							//然后分两种情况1、若该产生式只有A->B，将空加入A
							if (G[i].size() == 2) {
								it.second.insert('@');
								if (it.second.size() > size) ischanged = true;
							}
							//2、A->BC将FIRST(C)加入FIRST(A)，若C也能推空的情况在下一次循环中会再一次处理（类似于递归的思想）
							else {
								for (auto j = nonterminal[G[i][2]].begin(); j != nonterminal[G[i][2]].end(); j++) {
									it.second.insert(*j);
									if (it.second.size() > size) ischanged = true;
								}
							}
						}
					}
				}
			}
		}
	}
}

void show_First() {
	for (auto& it : nonterminal) {
		//遍历nonterminal的map，先输出nonterminal.fist，也就是一个终结符
		cout << "FIRST(" << it.first << "):";
		int size = it.second.size();
		//遍历second的size()大小，取出其中的值，这里存放的是first集合
		/*for (auto& iter1 : it.second)
			cout << iter1 << "	";*/
		for (auto iter = it.second.begin(); iter != it.second.end(); iter++) {
			cout << *iter << "	";
		}
		cout << endl;
	}
}

int main() {
	read_grammar2();
	//show_grammar2();
	deal_with_token();
	//show_token();
	get_First();
	show_First();
}