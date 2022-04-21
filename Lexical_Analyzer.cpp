#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <fstream>
#include <cstdio>
#include <stack>

#define debug cout
using namespace std;

ifstream input;//读入文件流（正规文法和源程序）
ofstream output;//输出文件流（token二元组）

/*定义一个关键字数组，sacn()读入，识别token串时
	若首字母是字母,则先遍历数组与依次与token比较判断是不是关键字
		若是关键字，break输出keywordtoken
		若不是，放入DFA判断哪是不是合法的标识符
*/
const char keyword[][10] = { 
	"int","float","double","char","bool" ,
	"if","else" ,"main" ,"void" ,"while" ,
	"break" ,"continue"  ,"long" ,"switch" ,
};

struct NFA_node {
	char node[100];

};

void create_NFA() {
	ifstream read("grammar3.txt");
	string content;
	while (read >> content) {
		//读三型文法的文件
		if (content[0] == '#') {
			if (content == "#endread") break;
			else continue;
		}
		//debug << content << endl;
	}
}

int main() {
	create_NFA();
	return 0;
}