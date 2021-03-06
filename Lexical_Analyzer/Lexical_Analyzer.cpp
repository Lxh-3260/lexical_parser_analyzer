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

const int MAX_NODES = 100;

ifstream input;//读入文件流（正规文法）
ofstream output;//输出文件流（token二元组）

set<char> weight;//读取正规文法的所有不重复的终结符（nfa边上权值）,在create_NFA函数中初始化，在NFA_to_DFA()中调用
set<char> C[MAX_NODES];//C为DFA每一个子集里的结点，下标从0开始，用于debugNFA_to_DFA的正确性
vector<int> dfa_final_state;//dfa的终态，在NFA_to_DFA()中调用，遍历一遍DFA的所有状态子集，有Z的子集push_back进这个vector

int now_dfa_state;//token放入dfa中跑时，当前的dfa_state，在dfa_can_run_final()函数里会用到，判断一个token串能否跑到终态以判断其合法性

vector<string> token_result;//存放token的最终结果

/*定义一个关键字数组，sacn()读入，识别token串时
	若首字母是字母,则先遍历数组与依次与token比较判断是不是关键字
		若是关键字，break输出line，token，keyword
		若不是，放入DFA判断哪是不是合法的标识符
*/
const char keyword[][10] = {
	"int","float","double","char","bool" ,
	"if","else" ,"main" ,"void" ,"while" ,
	"break" ,"continue" ,"return" ,
	"cin" , "cout" 
};//cin cout main 并不是关键字，方便判别，作为关键字来识别

//定义NFA和DFA的结构体
struct Edge_NFA {
	char start = '0';//start为开始结点
	char val = '0';//start为边上权值
	char end = '0';//end为指向的结点
};

struct NFA_node {
	set<char> node;//结点集合
	Edge_NFA edge[100];//边集
	int edge_num = 0;//边数
}nfa;

struct Edge_DFA {
	int start = 0;//start为开始结点
	char val = '0';//start为边上权值
	int end = 0;//end为指向的结点
};

struct DFA_node {
	set<int> node;//结点集合
	Edge_DFA edge[100];//边集
	int edge_num = 0;//边数
}dfa;

//初始化NFA，加入终态结点Z
void init_NFA() {
	nfa.node.insert('Z');
}

//读取3型文法，创建一个NFA，由于三型文法是右线性，只需要添加与终态Z之间的边即可
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
		//文法右边只有一个终结符直接到终态，有一个终结符一个非终结符，指向非终结符的那个状态
		if (content[4] == ' ') nfa.edge[nfa.edge_num++].end = 'Z';
		else nfa.edge[nfa.edge_num++].end = content[4];
		/*
		写报告时想到,在3型文法右侧只有一个终结符时更优的判断方法
		我原来是用加一个space空格的方式，使得每个文法长度一样，方便判断
		其实直接读取content.size()的方法也可以判断，写前面代码的时候没想到string可以直接调用size函数，写到dfa那块才想起来string可以知道size
		若直接用content.size(),判断起来更加简单而且写3型文法也没有限制了
		*/
		
		
		//初始化weight集合，为dfa中所有的弧上权值（后面因为要对所有终结符求弧转闭包，所以要用）
		weight.insert(content[3]);
	}
}

//debug时用于展示nfa结点
void show_NFA_node() {
	ofstream out;
	out.open("show_NFA.txt");
	debug << "NFA的所有结点如下:" << endl;
	out << "NFA的所有结点如下:" << endl;
	for (auto i = nfa.node.begin(); i != nfa.node.end(); i++) {
		debug << *i << endl;
		out << *i << endl;
	}
	out << "NFA的所有边如下:" << endl;
	for (int i = 0; i < nfa.edge_num; i++) {
		out << nfa.edge[i].start << "	" << nfa.edge[i].val << "	" << nfa.edge[i].end << endl;
	}
	out.close();
}

//debug时用于展示nfa的边
void show_NFA_edge() {
	debug << "NFA的所有边如下:" << endl;
	for (int i = 0; i < nfa.edge_num; i++) {
		debug << nfa.edge[i].start << "	" << nfa.edge[i].val << "	" << nfa.edge[i].end << endl;
	}
}

//集合移入函数，用于dfa求弧转闭包的时候状态移入集合的操作，对照书上的算法原理实现
set<char> move(set<char> I, char a, NFA_node N) {
	set<char> temp;
	for (auto it = I.begin(); it != I.end(); it++)
		for (int i = 0; i < N.edge_num; i++) {
			if (N.edge[i].start == *it && N.edge[i].val == a)
				temp.insert(N.edge[i].end);
		}
	return temp;
}

//求Epsilon状态闭包的函数，对照书上算法原理实现
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

//通过调用move和e_closure将NFA转化成DFA
void NFA_to_DFA() {
	ofstream out;
	out.open("show_DFA.txt");
	out << "NFA_TO_DFA Result:" << endl;
	cout << "NFA_TO_DFA Result:" << endl;
	bool marked[MAX_NODES] = { false };
	set<char> T0, T1;

	//注意这里要手动插入初态到T0，如果想测试不同的样例需要换不同的初态集合插入！
	T0.insert('L'); T0.insert('O'); T0.insert('I'); T0.insert('A'); T0.insert('S'); T0.insert('M');
	//T0.insert('0');//测试test文件中的的dfa正确性用的，对比书上p52的状态图检验其正确性

	//T1对T0做Epsilon状态闭包
	T1 = e_closure(T0, nfa);
	//T1作为第一个子集，开始求后续不重复的弧转子集闭包
	C[0] = T1;
	//开始求所有子集
	int i = 0;
	while (!C[i].empty() && marked[i] == false && i < MAX_NODES) {
		marked[i] = true;
		
		//输出DFA的每个子集C[i]
		out << "DFA的第" << i << "个子集为:";
		cout << "DFA的第" << i << "个子集为:";
		auto count_size = C[i].size();
		for (auto it = C[i].begin(); it != C[i].end(); it++) {
			out << *it;
			cout << *it;
			if (--count_size) {
				out << " , ";
				cout << " , ";
			}
		}
		out << endl;
		cout << endl;
		for (auto it = weight.begin(); it != weight.end(); it++) {//weight.size()为nfa总的val值数量 
			int j = 0;
			if (*it != '@') {
				//非空的弧转
				set<char> temp = e_closure(move(C[i], *it, nfa), nfa);//N为正规文法条数 ，G为Edge的start  val  end 
				if (!temp.empty()) {//C为DFA每一个集合里结点
					bool inC = false;
					int k = 0;
					while (!C[k].empty() && k < MAX_NODES) {
						//判断是否是重复子集
						if (temp == C[k]) {
							inC = true;
							break;
						}
						k++;
					}
					if (!inC) {
						//不是重复子集，把弧转新出现的子集插入C
						k = 0;
						while (!C[k].empty() && k < MAX_NODES)
						{
							k++;
						}
						C[k] = temp;
					}
					dfa.node.insert(i);//插入初态
					dfa.node.insert(k);//插入终态结点
					dfa.edge[dfa.edge_num].start = i;
					dfa.edge[dfa.edge_num].val = *it;
					dfa.edge[dfa.edge_num].end = k;
					dfa.edge_num++;
					out << i << "	input	" << *it << "	to	" << k << endl;
					cout << i << "	input	" << *it << "	to	" << k << endl;
					//debug << i << " " << k << " " << *it << endl;//https://csacademy.com/app/graph_editor/的输出格式，生成有向图用的
				}
				j++;
			}
		}
		i++;
	}
	//下面求出确定化后的终态
	/*
	步骤如下：
	1.C[i]存放的是DFA的第i个子集
	2.依次遍历C[i]，若为空说明已经遍历完
	3.非空子集则用迭代器遍历C内元素，若有Z的终态结点，则说明这个子集是DFA的终态
	*/
	i = 0;
	while (!C[i].empty()) {
		bool is_final_state = false;
		for (auto it = C[i].begin(); it != C[i].end(); it++) {
			if (*it == 'Z') {
				is_final_state = true;
				break;
			}
		}
		if (is_final_state) {
			dfa_final_state.push_back(i);
		}
		i++;
	}
	//输出DFA终态结点
	out << "Final state is : ";
	for (int a = 0; a < dfa_final_state.size(); a++) {
		out << dfa_final_state[a];
		cout << dfa_final_state[a];
		if (a != dfa_final_state.size() - 1) {
			out << " , ";
			cout << " , ";
		}
	}
	out << endl;
	cout << endl;
	out.close();
}

//debug用于检查dfa所有的结点
void show_DFA_node() {
	debug << "DFA的所有结点如下:" << endl;
	for (auto i = dfa.node.begin(); i != dfa.node.end(); i++) {
		debug << *i << endl;
	}
}

//debug用于检查dfa所有的边
void show_DFA_edge() {
	debug << "DFA的所有边如下:" << endl;
	for (int i = 0; i < dfa.edge_num; i++) {
		debug << dfa.edge[i].start << "	" << dfa.edge[i].val << "	" << dfa.edge[i].end << endl;
	}
}

//将扫描进来的token放进dfa跑，看能不能到终态，能跑到终态的才能说明其是合法的token，否则就是错误的token
bool token_can_run_final(string str) {
	if (str.size() == 0) {
		for (int i = 0; i < dfa_final_state.size(); i++) {
			//若能在dfa跑到终态
			if (now_dfa_state == dfa_final_state[i])
			{
				//每次递归结束要记得把now_dfa_state = 0，回到初态，否则只能判断一次了
				now_dfa_state = 0;
				return true;
			}
		}
	}
	//取token的第一个字符，并删去第一个字符后，用递归法判断能否跑到终态
	char ch = str[0];
	string str_deletefirst;
	for (int i = 1; i < str.size(); i++) str_deletefirst += str[i];
	for (int i = 0; i < dfa.edge_num; i++) {
		if (dfa.edge[i].start == now_dfa_state && dfa.edge[i].val == ch) {
			now_dfa_state = dfa.edge[i].end;
			return token_can_run_final(str_deletefirst);
		}
	}
	//每次递归结束要记得把now_dfa_state = 0，回到初态，否则只能判断一次了
	now_dfa_state = 0;
	return false;
}

//读入源程序
void scan() {
	ifstream read_program("source_program.txt");
	//一行一行地读入源程序
	string line;
	int line_number = 1;
	while (getline(read_program,line)) {
		int i = 0;
		//去除源程序每行最前面的空格
		while (line[i] == ' ') {
			i++; 
		}
		int flag = i;
		//一个vector数组，存放每一行要识别的token
		vector<string> token;
		for (; i < line.size(); i++) {
			string temp_token;
			if (i == line.size() - 1) {
				string s;
				s+= line[i];
				token.push_back(s);
			}
			if (line[i] == ' ') {
				for (int j = flag;j<i; j++) {
					temp_token += line[j];
				}
				token.push_back(temp_token);
				flag = i + 1;
			}
		}
		i = 0;
		//用于debug输出所有token查看的读取是否正确
		/*for (int k = 0; k < token.size(); k++) {
			debug << token[k] << " ";
		}
		cout << endl;*/
		//开始处理每行的token
		for (int temp1 = 0; temp1 < token.size(); temp1++) {
			//token[temp1]表示每一行的第(temp1-1)个待识别的token
			//求出keyword二维数组总共有多少行
			int all = sizeof(keyword) / sizeof(char);
			int column = sizeof(keyword[0]) / sizeof(keyword[0][0]);
			int row = all / column;
			//一个bool型变量，假如一个token 已经被判断过就要continue判断下一个token
			bool is_judged = false;
			//1、先判断关键字keyword
			//直接与关键字数组比较即可
			for (int temp2 = 0; temp2 < row; temp2++) {
				if (token[temp1] == keyword[temp2]) {
					is_judged = true;
					cout << line_number << "," << token[temp1] << "," << "keyword" << endl;

					output << line_number << "," << token[temp1] << "," << "keyword" << endl;
				}
			}
			if (is_judged) continue;
			//2、再判断界符limiter
			//界符由于只有一位，直接判断即可，写文法的时候没考虑到这个，所以多写了界符的文法，其实界符不用dfa也能识别
			if (token[temp1] == "," || token[temp1] == ";" || token[temp1] == "[" || token[temp1] == "]"
				|| token[temp1] == "(" || token[temp1] == ")" || token[temp1] == "{" || token[temp1] == "}") {
				is_judged = true;
				cout << line_number << "," << token[temp1] << "," << "limiter" << endl;

				output << line_number << "," << token[temp1] << "," << "limiter" << endl;
			}
			if (is_judged) continue;
			//3、再判断操作符operator
			//先读operator的第一位，若是运算符，则放进dfa看看能不能跑到终态，若能则说明是合法的运算符
			//（因为有二元运算符，所以不能只靠读取一个字符识别）
			if (token[temp1][0] == '+' || token[temp1][0] == '-' || token[temp1][0] == '*'
				|| token[temp1][0] == '/' || token[temp1][0] == '%' || token[temp1][0] == '^'
				|| token[temp1][0] == '=' || token[temp1][0] == '>' || token[temp1][0] == '<') {
				is_judged = true;
				if (token_can_run_final(token[temp1])) {
					cout << line_number << "," << token[temp1] << "," << "operator" << endl;

					output << line_number << "," << token[temp1] << "," << "operator" << endl;
				}
				else {
					cout << line_number << "," << token[temp1] << "," << "wrong operator" << endl;

					output << line_number << "," << token[temp1] << "," << "wrong operator" << endl;
				}
			}
			if (is_judged) continue;
			//4、再判断标识符identifier
			string str_token = token[temp1];
			for (int temp2 = 0; temp2 < token[temp1].size(); temp2++) {
				if ((token[temp1][temp2] >= 'a' && token[temp1][temp2] <= 'z' && token[temp1][temp2] != 'i' && token[temp1][temp2] != 'e') ||
					(token[temp1][temp2] >= 'A' && token[temp1][temp2] <= 'Z' && token[temp1][temp2] != 'i' && token[temp1][temp2] != 'e')) token[temp1][temp2] = 'a';
				if (token[temp1][temp2] <= '9' && token[temp1][temp2] >= '1') token[temp1][temp2] = 'c';
			}
			//以字母或者下划线开头的，一定是标识符，判断合法性即可
			if (token[temp1][0] == '_' || token[temp1][0] == 'a') {
				if (token_can_run_final(token[temp1])) {
					cout << line_number << "," << str_token << "," << "identifier" << endl;

					output << line_number << "," << str_token << "," << "identifier" << endl;
				}
				else {
					cout << line_number << "," << str_token << "," << "wrong identifier" << endl;

					output << line_number << "," << str_token << "," << "wrong identifier" << endl;
				}
			}
			//以数字开头的，有可能是常量number，有可能是非法标识符，遍历一下
			//5、最后判断常量number
			bool is_wrong_identifier = false;
			if (token[temp1][0] == 'c' || token[temp1][0] == '0') {
				for (int l = 0; l < token[temp1].size(); l++) {
					if (token[temp1][l] == 'a') {
						is_wrong_identifier = true;
						break;
					}
				}
				if (is_wrong_identifier) {
					//如果是错误的标识符
					cout << line_number << "," << str_token << "," << "wrong identifier" << endl;

					output << line_number << "," << str_token << "," << "wrong identifier" << endl;
					continue;
				}
				else {
					//不是错误的标识符，则是常量，用dfa判断常量合法性
					if (token_can_run_final(token[temp1])) {
						cout << line_number << "," << str_token << "," << "number" << endl;

						output << line_number << "," << str_token << "," << "number" << endl;
					}
					else {
						cout << line_number << "," << str_token << "," << "wrong number" << endl;

						output << line_number << "," << str_token << "," << "wrong number" << endl;
					}
				}
			}
		}
		line_number++;
	}
}

int main() {
	create_NFA();
	show_NFA_node();//输出至show_NFA.txt
	show_NFA_edge();
	NFA_to_DFA();//输出至show_DFA.txt
	show_DFA_node();
	show_DFA_edge();
	
	//测试正规文法和token的输入部分
	/*string token;
	while (cin >> token)
	{
		debug << token_can_run_final(token) << endl;
	}*/
	output.open("token_output.txt");
	scan();
	output.close();
	getchar();
	return 0;
}
