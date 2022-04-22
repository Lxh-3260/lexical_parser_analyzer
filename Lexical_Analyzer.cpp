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

using namespace std;
#define debug cout
const int MAX_NODES = 100;
ifstream input;//读入文件流（正规文法和源程序）
ofstream output;//输出文件流（token二元组）

set<char> weight;//读取正规文法的所有终结符（nfa边上权值）,在create_NFA函数中初始化

/*定义一个关键字数组，sacn()读入，识别token串时
	若首字母是字母,则先遍历数组与依次与token比较判断是不是关键字
		若是关键字，break输出keywordtoken
		若不是，放入DFA判断哪是不是合法的标识符
*/
const char keyword[][10] = { 
	"int","float","double","char","bool" ,
	"if","else" ,"main" ,"void" ,"while" ,
	"break" ,"continue" ,"return" ,
};

struct Edge {
	char start='0';//start为开始结点
	char val='0';//start为边上权值
	char end='0';//end为指向的结点
};

struct NFA_node {
	set<char> node;//结点集合
	Edge edge[100];//边集
	int edge_num = 0;//边数
}nfa;

//初始化NFA，加入终态结点Z
void init_NFA() {
	nfa.node.insert('Z');
}

//读取3型文法，创建一个NFA
void create_NFA() {
	init_NFA();
	ifstream read("grammar3.txt");
	string content;
	while (getline(read, content)) {
		//读三型文法的文件#表示那行是注释，跳过，读到#endread表示文件已读完结束
		if (content[0] == '#') {
			if (content == "#endread") break;
			else continue;
		}
		//debug << content << endl;
		bool isexist = false;
		//使用迭代器进行遍历用c++代码实现很复杂，用auto作为迭代指针
		//这里用auto是因为遍历set迭代器的类型很长(set<char>::iterator it;)
		for (auto i = nfa.node.begin(); i != nfa.node.end(); i++)  
		{
			if (content[0] == *i) {
				//文法左侧的非终结符已经在nfa.node集合里面
				isexist = true;
				break;
			}
		}
		if (!isexist) {
			//文法左侧的非终结符没在nfa.node集合里面，则插入集合
			nfa.node.insert(content[0]);
		}
		//不管这条三型文法的开始结点在不在集合，都要插入这条边
		nfa.edge[nfa.edge_num].start = content[0];
		nfa.edge[nfa.edge_num].val = content[3];
		weight.insert(content[3]);
		if (content[4] == ' ') nfa.edge[nfa.edge_num++].end = 'Z';
		else nfa.edge[nfa.edge_num++].end = content[4];
	}
}

//debug时用于展示nfa的边
void show_NFA_edge() {
	for (int i = 0; i < nfa.edge_num; i++) {
		debug << nfa.edge[i].start << "	" << nfa.edge[i].val << "	" << nfa.edge[i].end << endl;
	}
}

//debug时用于展示nfa结点
void show_NFA_node() {
	for (auto i = nfa.node.begin(); i != nfa.node.end(); i++) {
		debug << *i << endl;
	}
}

//集合移入函数，用于dfa求弧转闭包的时候状态移入集合的操作
set<char> move(set<char> I, char a, NFA_node N) {
	set<char> temp;
	for (auto it = I.begin(); it != I.end(); it++)
		for (int i = 0; i < N.edge_num; i++) {
			if (N.edge[i].start == *it && N.edge[i].val == a)
				temp.insert(N.edge[i].end);
		}
	return temp;
}

//求Epsilon状态闭包的函数
set<char> e_closure(set<char> I, NFA_node N) {
	set<char> temp = I;
	stack<char> st;
	for (auto it = temp.begin(); it != temp.end(); it++)
	{
		st.push(*it);
	}
	char t;
	while (!st.empty()) {
		t = st.top();
		st.pop();
		for (int i = 0; i < N.edge_num; i++) {
			if (N.edge[i].start == t && N.edge[i].val == '@') {
				temp.insert(N.edge[i].end);
				st.push(N.edge[i].end);
			}
		}
	}
	return temp;
}

void NFA_to_DFA() {
	cout << endl << "确定后的DFA:" << endl;
	bool marked[MAX_NODES] = { false };
	//for (int i = 0; i < MAX_NODES; i++) marked[i] = false;
	set<char> C[MAX_NODES];//C为DFA每一个集合里结点 
	/*char s0 = nfa.edge[0].start;*/
	set<char> T0, T1;
	T0.insert('L'); T0.insert('O'); T0.insert('I'); T0.insert('A'); T0.insert('S'); T0.insert('M');

	T1 = e_closure(T0, nfa);
	C[0] = T1;
	int i = 0;
	while (!C[i].empty() && marked[i] == false && i < MAX_NODES) {
		marked[i] = true;
		//下面被注释代码可用于输出图中求出来的集合

		set<char>::iterator it;
		//cout << i << ":";
		printf("第%d个集合为：", i);
		for (it = C[i].begin(); it != C[i].end(); it++)
			cout << *it << ",";
		cout << endl;

		for (auto it=weight.begin(); it != weight.end(); it++) {//weight.size()为nfa总的val值数量 
			int j = 0;
			set<char> temp = e_closure(move(C[i], *it, nfa), nfa);//N为正规文法条数 ，G为Edge的start  val  end 
			if (!temp.empty()) {//C为DFA每一个集合里结点
				bool inC = false;
				int k = 0;
				while (!C[k].empty() && k < MAX_NODES) {
					if (temp == C[k]) {
						inC = true;
						break;
					}
					k++;
				}
				if (!inC) {
					k = 0;
					while (!C[k].empty() && k < MAX_NODES)
					{
						k++;
					}
					C[k] = temp;
				}
				cout << i << "	input	" << *it << "	to	" << k << endl;
			}
			j++;
		}
		i++;
	}
	//下面求出确定化后的终态
	cout << "终态为:";
	i = 0;
	while (!C[i].empty()) {
		bool is_final_state = false;
		for (auto it = C[i].begin(); it != C[i].end(); it++) {
			if (*it == 'Z') {
				is_final_state = true;

				break;
			}
		}
		if (is_final_state) cout << i << ",";
		i++;
	}
	cout << endl;
}

int main() {
	create_NFA();
	//show_NFA_edge();
	//show_NFA_node();
	NFA_to_DFA();
	return 0;
}