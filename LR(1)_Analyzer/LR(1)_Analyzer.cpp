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

vector<vector<char>> G;//存放文法，一个vector二维数组，一行存放一行文法，具体的vector嵌套用法可以去搜索一下“C++ 中vector的嵌套使用”这篇博客
map<char, set<char>> terminal;//存放终结符到其FIRST集合的映射，terminal.first表示map的前一项，为终结符对应的char，terminal.second表示map的后一项，为该终结符的FIRST集
map<char, set<char>> nonterminal;//存放非终结符到其FIRST集合的映射，nonterminal.first表示map的前一项，为终结符对应的char，nonterminal.second表示map的后一项，为该终结符的FIRST集

vector<string> token;//读入词法分析输出的token，并将其转换成我写的二型文法所能识别的符号串，即终结符（包括number，identifer，type，keyword，界符，运算符）

map< map<string, char>, string> table; //LR分析表

//项目集
struct Project {
	vector<vector<char>> project;//项目集I，二维数组，行表示该项目集有多少行产生式，列表示产生式的各个字符
	vector<set<char>> search_forward;//项目集I的每条产生式对应的向前搜索符号集，由于每个终结符在一条产生式的向前搜索符号集只能出现一次，故set集合更符合其数据结构
	map<char, int> go;//每个项目输入一个char字符，要转移到的另一个项目编号
};
vector<Project> clousure;//clousure表示所有项目集的闭包

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
	//debug时输错了ifstream的文件名导致vector out of range的严重错误，故加上exit(0)，当没读到文件时直接退出
	if (G.empty()) exit(0);
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
	bool ischanged = true;//每次insert操作会看该终结符的FIRST.size()是否改变，只要改变了，就要一直循环再找一次
	while (ischanged) {
		ischanged = false;
		for (auto& it : nonterminal) {
			//对于每个非终结符遍历一次所有文法，it.first为当前非终结符，it.second为该终结符的First集合
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
							//1. {FIRST(B)-{@}}
							for (auto j = nonterminal[G[i][1]].begin(); j != nonterminal[G[i][1]].end(); j++) {
								if (*j != '@') it.second.insert(*j);
								if (it.second.size() > size) ischanged = true;
							}
							//2. 然后分两种情况
							//2.1 若该产生式只有A->B，即C为空，{FIRST(C)}='@'，将空加入A
							if (G[i].size() == 2) {
								it.second.insert('@');
								if (it.second.size() > size) ischanged = true;
							}
							//2.2 A->BC将FIRST(C)加入FIRST(A)，若C也能推空的情况在下一次循环中会再一次处理（类似于递归的思想）
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

//debug时展示FIRST集，并验证其正确性
void show_First() {
	for (auto& it : nonterminal) {
		//遍历nonterminal的map，先输出nonterminal.fist，也就是一个终结符
		debug << "FIRST(" << it.first << "):";
		int size = it.second.size();
		//遍历second的size()大小，取出其中的值，这里存放的是first集合
		/*for (auto& iter1 : it.second)
			cout << iter1 << "	";*/
		for (auto iter = it.second.begin(); iter != it.second.end(); iter++) {
			debug << *iter << "	";
		}
		debug << endl;
	}
}

//根据G中读取的二型文法得到项目集族，注：注释中的项目集==闭包，项目==产生式，项目集族也就是所有闭包的集合，写注释的时候混用了，希望读者能看懂
void get_Clousure() {
	int i = 0;//每个项目集的编号
	Project p;
	clousure.push_back(p);
	while (true) {
		if (i == clousure.size()) break;//没有新的闭包产生时，跳出循环
		if (i == 0) {
			//先初始化I0项目集的第一条产生式S’->·S,#
			vector<char> temp1(G[0]);//temp1存新产生式
			temp1.insert(temp1.begin() + 1, ' ');//用空格space代替·，因为·是圆角字符，无法被字符的ASCII码识别，后文所有的项目集和闭包中的空格都是表示·
			clousure[0].project.push_back(temp1);
			set<char> temp2;//temp2存新向前搜索符号集
			temp2.insert('#');
			clousure[0].search_forward.push_back(temp2);
		}

		//求每个项目集第一条产生式扩展出来的闭包项目
		for (int j = 0; j < clousure[i].project.size(); j++) {
			//遍历已有项目集，j为每个项目集中的第j个项目
			for (int k = 0; k < clousure[i].project[j].size(); k++) {
				//遍历该项目集，找到·的位置，k为每个项目中各个终结符非终结符的编号
				if (clousure[i].project[j][k] == ' ') {
					//找到了·的位置，开始从·的后面一个终结符生成新的产生式
					if (k == clousure[i].project[j].size() - 1) {
						//·在产生式的最后，比如说S'->S·,#  那么这条产生式不能推出新的产生式
						//故该项目集的·不需要处理，break处理下一个项目集
						break;
					}
					for (int l = 0; l < G.size(); l++) {
						//·在产生式的中间的情况，比如说A->α·Bβ,a
						//遍历所有的二型文法G，查找·后面的非终结符所有的产生式，并将新的产生式加入项目集
						if (G[l][0] == clousure[i].project[j][k + 1]) {
							vector<char> temp1(G[l]);
							temp1.insert(temp1.begin() + 1, ' ');
							//生成新的产生式后不能直接插入到当前的project中
							//先判断该新的项目是否存在在当前项目集，若存在则记录下编号，若不存在则插入
							int project_is_exist = 0;
							for (int m = 0; m < clousure[i].project.size(); m++) {
								if (temp1 == clousure[i].project[m]) {
									project_is_exist = m;
									break;
								}
							}
							if(project_is_exist == 0) clousure[i].project.push_back(temp1);
							//接下里开始处理向前搜索符号集
							set<char> temp2;//用来保存暂时的向前搜索符号集，后面用于push_back的
							/*
							形如产生式：A->α·Bβ,a  计算向前搜索符号集，即计算FIRST(βa)，分4种情况：
							1.该产生式没有β，产生式为A->α·B,a，向前搜索符号集为{a}
							2.β的第一个字符为终结符{b}，产生式为A->α·Bb,a，那么向前搜索符号集为该终结符{b}
							
							3.β的第一个字符为非终结符C，产生式为A->α·BC,a，且该非终结符能推出空,向前搜索符号集为FIRST(C)-{'@'}并上{γa}，其中γ为去除β的第一个符号C后的字符串
							4.β的第一个字符为非终结符C，产生式为A->α·BC,a，且该非终结符不能推出空,向前搜索符号集为FIRST(C)
							*/
							bool deduce_empty = true;//判断β的第一个字符C能不能推出空
							int n = 0;//n记录去除β的第n个字符
							while (deduce_empty) {
								deduce_empty = false;
								if (k + n + 1 == clousure[i].project[j].size() - 1) {
									//第一种情况β为空
									for (auto it : clousure[i].search_forward[j]) temp2.insert(it);
								}
								else if (terminal.find(clousure[i].project[j][k + n + 2]) != terminal.end()) {
									//第二种情况，首字符为终结符
									temp2.insert(clousure[i].project[j][k + n + 2]);
								}
								else {
									//第三、四种情况β的第一个字符为非终结符
									//找到C的FIRST集中所有的终结符
									set<char> temp_nonter(nonterminal.find(clousure[i].project[j][k + n + 2])->second);//注意find找到的是一个地址，迭代器的用法，不是一个值，所以不能用.second而是用->second
									for (auto it : temp_nonter) {
										if (it == '@') {
											//如果第一个字符能推出空
											deduce_empty = true;//再进行一次循环，找FIRST(γa)并插入
											n++;
										}
										else {
											//如果第一个字符不能推出空
											temp2.insert(it);
										}
									}
								}
							}
							/*
							现在新项目clousure[i].project[j]的向前搜索符号集暂存在set<char> temp2中
							对向前搜索符号集的处理分为2种：
							1.temp1原先不在该项目集中，根据原产生式后面部分的FIRST集合，来确定其向前搜索符号集
							2.temp1原先在项目集中，根据原产式，得到新的向前搜索符号集插入原先产生式的向前搜索符号集合
							*/
							if (project_is_exist == 0) {
								//第一种情况，temp1不在项目集中
								clousure[i].search_forward.push_back(temp2);
							}
							else {
								//第一种情况，temp1在项目集中
								for (auto it : temp2) {
									clousure[i].search_forward[project_is_exist].insert(it);
								}
							}
						}
					}
					
					//已经处理过这个项目集的·了，break处理下一个项目集
					break;
				}
			}
		}

		//判断该闭包是否是已经出现的闭包,并计算闭包之间相互转换的边
		for (int j = 0; j < clousure[i].project.size(); j++) {
			//遍历本项目集
			for (int k = 0; k < clousure[i].project[j].size(); k++) {
				//扫描每个产生式，找到·的位置
				if (clousure[i].project[j][k] == ' ') {
					//debug时发现project后面，少写了一个[j]，导致vector溢出，这么小的错误debug了半天
					//因为现在是对第i个项目集（闭包）的第j个项目（产生式）进行遍历，找到他·的位置，所以显然要用project[j].size()
					if (k == clousure[i].project[j].size() - 1) {
						//如果·在产生式最后，那么这个产生式不会有新的闭包，break，处理下一个产生式
						break;
					}
					//否则·不在产生式的最后，则将点后移一位
					//然后计算闭包之间的边，生成新的闭包
					vector<char> new_clousure_project(clousure[i].project[j]);//新的产生式（有原产生式推出，故初始化为源产生式clousure[i].project[j]）
					new_clousure_project[k] = new_clousure_project[k + 1];//new_clousure_project[k]存放的是待移入字符
					new_clousure_project[k + 1] = ' ';
					char ch = new_clousure_project[k];//ch存放待移入字符(后面嵌套太多了，用new_clousure_project[k]很混乱)
					set<char> new_clousure_search_forward(clousure[i].search_forward[j]);//新的向前搜索符号集（与原产生式向前搜索符号集一样）
					bool is_new_clousure = false;//判断新生成的项目是否是新的闭包（用于判断要不要生成新闭包）
					for (int m = 0; m < clousure.size(); m++) {
						//遍历所有的闭包，看看该闭包是否已经出现过（项目集相等且向前搜索符号集也相等）
						for (int n = 0; n < clousure[m].project.size(); n++) {
							//遍历每个闭包的所有项目集
							is_new_clousure = false;
							if (new_clousure_project == clousure[m].project[n]) {
								//这里的下标也是debug了半天，应该是跟第一条产生式的向前搜索符号集size对比，看是否一样，因为如果跟第一条产生式的project和search_forward都一样，就能知道闭包必然也一样
								//而不是跟整个项目集搜索符号集的大小比，第一次写的时候search_forward后面少写了个[0]，导致debug了半天浪费了时间，写程序还是应该先写思路伪码，不能着急
								if (clousure[m].search_forward[0].size() != new_clousure_search_forward.size()) {
									is_new_clousure = true;
									break;
								}
								auto it1 = clousure[m].search_forward[0].begin();
								for (auto it2 : new_clousure_search_forward) {
									if (it2 != *it1) {
										is_new_clousure = true;
										break;
									}
									/*if (it1 == clousure[m].search_forward[0].end()) {
										is_new_clousure = false;
										break;
									}*/
									it1++;
								}
								if (is_new_clousure == false) {
									clousure[i].go[ch] = m;
									break;
								}
							}
							else is_new_clousure = true;

							if (is_new_clousure == false) break;
						}
						if (is_new_clousure == false) break;
					}
					/*
					例如这种情况，第一次第一行T的go函数会计算出来，第二次要把第二行的产生式加入第一次的go函数指向的那个闭包
					E->·T，)/+
					T->·T*F，)/+/* 
					*/
					if (clousure[i].go.count(new_clousure_project[k]) != 0 && is_new_clousure) {
						clousure[clousure[i].go[ch]].project.push_back(new_clousure_project);
						clousure[clousure[i].go[ch]].search_forward.push_back(new_clousure_search_forward);
						break;
					}
					//如果是没出现过的新的闭包，插入该闭包
					if (is_new_clousure == true) {
						Project new_clousure;
						new_clousure.project.push_back(new_clousure_project);
						new_clousure.search_forward.push_back(new_clousure_search_forward);
						clousure.push_back(new_clousure);
						//go是一个char到int的映射，表示输入对应的char，该闭包会跳转至哪个闭包，
						//比如A->α·BC,a，那么他的输入符号就是B，也就是对应new_clousure_project[k]
						//由于输入的符号是new_clousure_project[k]，新的闭包是clousure.size()-1
						clousure[i].go[ch] = clousure.size() - 1;
					}
				}
			}
		}
		
		i++;
	}
}

//debug时展示项目集和他们之间相互转化的函数，并验证其正确性
void show_Clousure() {
	for (int i = 0; i < clousure.size(); i++) {
		printf("I%d:\n", i);
		for (int j = 0; j < clousure[i].project.size(); j++) {
			for (int k = 0; k < clousure[i].project[j].size(); k++) {
				if (clousure[i].project[j][k] != ' ')cout << clousure[i].project[j][k];
				else cout << "·";
				if (k == 0) cout << "->";
			}
			cout << ',';
			int count = 0;
			for (auto it = clousure[i].search_forward[j].begin(); it != clousure[i].search_forward[j].end(); it++) {
				cout << *it;
				
				if (count != clousure[i].search_forward[j].size() - 1) cout << '/';
				count++;
			}
			cout << endl;
		}
		for (auto& it : clousure[i].go) {
			cout <<"input char: " << it.first <<" to clousure I" << it.second << endl;
		}
		cout << endl;
	}
}

//得到项目集后，计算LR(1)文法的ACTION GOTO表
bool get_LR1Table() {
	/*
	分为四种情况（思想方法见清华大学教材p146）
	1. S’->S·,#      属于Ik，则ACTION表中[k,#]为acc
	2. A->β·,a       属于Ik，则ACTION表中[k,a]为rj，表示用第j条产生式规约
	3. A->α·aβ,b    属于Ik，且Ik移进a转移到Ij，则ACTION表中[k,a]为Sj，表示把移入符号a和状态j分别移入文法符号栈和状态符号栈
	4. A->α·Bβ,a    属于Ik，且Ik移进B转移到Ij，则GOTO表中[k,B]为j，表示置当前文法符号栈顶为A，状态栈顶为j
	*/
	for (int i = 0; i < clousure.size(); i++) {
		for (int j = 0; j < clousure[i].project.size(); j++) {
			for (int k = 0; k < clousure[i].project[j].size(); k++) {
				if (clousure[i].project[j][k] == ' ') {
					//找到每条文法的·
					if (k == clousure[i].project[j].size() - 1) {
						//若·在文法的最后，要对式子进行规约
						//分为1 2两种情况
						if (clousure[i].project[j][0] == 'Z') {
							//对应最上面注释的第一种情况S’->S·,#
							map<string, char> m;
							string state = to_string(i);
							m[state] = '#';
							if (table.find(m) != table.end() && table[m] != "acc") {
								cout << "error";
								return false;
							}
							else {
								table[m] = "acc";
							}
						}
						else {
							//对应最上面注释的第二种情况A->β·,a
							int G_num = 0;
							for (int x = 0; x < G.size(); x++) {
								vector<char> y(clousure[i].project[j]);
								y.pop_back();//clousure[i].project[j]比二型文法多了最后的一个·，在vector里为space空格，pop出来便可以直接比较
								if (y == G[x]) {
									G_num = x;
									break;
								}
							}
							map<string, char> m;
							string state = to_string(i);
							for (auto it : clousure[i].search_forward[j]) {
								m[state] = it;
								if (table.find(m) != table.end() && table[m] != "r" + to_string(G_num)) {
									cout << "error";
									return false;
								}
								else {
									table[m] = "r" + to_string(G_num);
								}
							}
						}
					}
					else {
						//否则就是移进
						//分为3 4两种情况
						char next_ch = clousure[i].project[j][k + 1];//·的下一个字符，判断是终结符还是非终结符，分为情况三四
						if (terminal.find(next_ch) != terminal.end()) {
							//A->α·aβ,b，对应上面的第三种情况
							map<string, char> m;
							m[to_string(i)] = next_ch;
							if (table.find(m) != table.end() && table[m] != "S" + to_string(clousure[i].go[next_ch])) {
								cout << "error";
								return false;
							}
							else {
								table[m] = "S"+to_string(clousure[i].go[next_ch]);
							}
						}
						else {
							//A->α·Bβ,a，对应上面的第四种情况
							map<string, char> m;
							m[to_string(i)] = next_ch;
							if (table.find(m) != table.end() && table[m] != to_string(clousure[i].go[next_ch])) {
								cout << "error";
								return false;
							}
							else {
								table[m] = to_string(clousure[i].go[next_ch]);
							}
						}
					}
					
					//分析完这个·，就是分析完这条项目，break处理下一条项目
					break;
				}
			}
		}
	}
	return true;
}

void show_LR1Table() {
	
}
int main() {
	read_grammar2();
	//show_grammar2();
	deal_with_token();
	//show_token();
	get_First();
	//show_First();
	get_Clousure();
	//show_Clousure();
	get_LR1Table();
	//show_LR1Table();
}