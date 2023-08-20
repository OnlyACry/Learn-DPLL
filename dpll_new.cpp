#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
#include <sstream>
#include <stack>
#include <chrono>
#include <algorithm>
#include <set>
#include <map>

using namespace std;

struct Save_C
{
    bool tf;        //句子满足性
    int length;     //句子长度改变
}Save_C;

class S_C
{
    public:   
    vector<int> v;      //记录改变的文字，其赋值为此文字原本在整个算例中出现的次数
    map<int, struct Save_C> all_change_c;   //记录修改之前的句子

    S_C(){}

    S_C(const S_C &sc)
    {
        v = sc.v;
        all_change_c = sc.all_change_c;
    }
}

//输入算例
class F
{
    public:
    vector<int> literals_pos;   //文字 0-false 1-true -1-undesign -2-未出现
    vector<int> literals_cnt;   //文字出现次数
    vector<int> clauses_literal_cnt;    //子句长度
    vector<bool> clause_tf;     //子句是否满足
    vector<vector<int>> clauses;    //子句

    F(){}

    F(const F &f)
    {
        literals_pos = f.literals_pos;
        clauses = f.clauses;
        literals_cnt = f.literals_cnt;
        clauses_literal_cnt = f.clauses_literal_cnt;
        clause_tf = f.clause_tf;
    }
};

string file = "sud00021";
int all_literals, all_clauses;

void init(F &f);
bool CheckNonClauses(F f);
bool CheckSatisfy(F f);
int ChooseLiteral(F f);
int ChooseLiteral2(F f);
bool UP(F &f, S_C &sc); 
void Find_UP(F f, int& signal_clause, int &num1, int &num2);
void Print(F f, bool satisfy, int64_t t);
void InsertClauses(F &f, int signal_clause);    //插入单子句
void Simplify_clauses(F &f, int signal_clause, S_C &sc);
void BackToPrevious(F &f, S_C sc);
void Updatef_sc(F &f, S_C &sc, int i, int signal_clause);

int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    S_C sc;
    stack<S_C>  save_stack;
    stack<int> flip_v;
    
    bool flag = false;
    int i = 0, signal_clause;

    init(f);
    do{
        if(UP(f, sc))
        {
            signal_clause = ChooseLiteral2(f);
            if(signal_clause == -1) {flag = true; break;}

            save_stack.push(sc);
            flip_v.push(signal_clause);
        }
        else
        {
            //返回到上一步
            if(save_stack.empty()) break;
            
            sc = save_stack.top();
            save_stack.pop();
            BackToPrevious(f, sc);
            //翻转变元
            signal_clause = flip_v.top();
            InsertClauses(f, -signal_clause);
            flip_v.pop();
        }
    }while(!save_stack.empty())

    if(flag)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        Print(f, true, duration);
    }
    else
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        printf("s -1");
        printf("\nt %ld\n", duration);
    }

    return 0;
}

void init(F &f)
{
    string s = "test/M/" + file + ".cnf";
    ifstream file(s);
    if(!file){
        cout<<"Failed to open the file"<<endl;
        return;
    }

    string line;
    int literal, i=0, j=0;
    while(getline(file, line)){
        istringstream iss(line);
        if(line[0] == 'c') continue;    //注释行跳过
        if(line[0] == 'p')
        {
            string p, cnf;
            iss >> p >> cnf >> all_literals >> all_clauses;
            f.literals_pos.clear();
            f.literals_pos.resize(all_literals, -1);
            f.literals_cnt.clear();
            f.literals_cnt.resize(all_literals);
            f.clauses.clear();
            f.clauses.resize(all_clauses);
            f.clauses_literal_cnt.clear();
            f.clauses_literal_cnt.resize(all_clauses, 0);
            f.clause_tf.clear();
            f.clause_tf.resize(all_clauses, false);
            continue;
        }

        while(iss >> literal)
        {
            if(literal == 0) {
                f.clauses_literal_cnt[i++] = j;   //子句中文字数量
                j = 0;
                continue;
            }
            if(literal > 0) f.literals_cnt[literal-1]++;
            else if(literal < 0) f.literals_cnt[abs(literal) - 1]++;
            f.clauses[i].push_back(literal);
            j++;
        }
    }
}

void initsc(S_C &sc)
{
    sc.v.resize(all_literals, -1);
    sc.all_change_c.resize(all_clauses);
}

//返回最多次文字出现的下标
int ChooseLiteral(F f)
{
    return max_element(f.literals_cnt.begin(), f.literals_cnt.end()) - f.literals_cnt.begin() + 1;
}

//返回句子最短的未赋值文字
int ChooseLiteral2(F f)
{
    //返回最短子句中出现次数最多的文字
    //find shorest clause
    int min = INT16_MAX, index, num;
    for(int i=0; i<f.clauses.size(); i++)
    {
        if(f.clause_tf[i] == true || f.clauses_literal_cnt[i] == 0) continue;
        if(f.clauses_literal_cnt[i] < min)
        {
            min = f.clauses_literal_cnt[i];
            index = i;
        }
    }
    int max = INT16_MIN, n = -1;
    for(int j=0; j<f.clauses[index].size(); j++)
    {
        n = f.clauses[index][j];
        if(f.literals_pos[abs(n) - 1] == -1 && f.literals_cnt[abs(n) - 1] > max)
        {
            num = n;
            max = f.literals_cnt[abs(n) - 1];
        }
    }
    return num;
}

void Updatef_sc(F &f, S_C &sc, int i, int signal_clause)
{
    struct Save_C pov_clause;
    //首先检测是否已有键值
    if(sc.all_change_c.find(i) != sc.all_change_c.end()) return;    //存在映射中，不更新

    pov_clause.length = f.clauses_literal_cnt[i];
    pov_clause.tf = f.clause_tf[i];
    sc.v[abs(signal_clause) - 1] = f.clauses_literal_cnt[abs(signal_clause) - 1];
    sc.all_change_c[i] = pov_clause;
}

void BackToPrevious(F &f, S_C sc)
{
    int index;
    struct Save_C back_c;
    for (map<int, struct Save_C>::iterator it = myMap.begin(); it != myMap.end(); ++it) 
    {
        index = it->first;
        back_c = it->second;
        f.clause_tf[index] = back_c.tf;
        f.clauses_literal_cnt[index] = back_c.length;
    }
    f.clauses.pop_back();
    f.clause_tf.pop_back();
    f.clauses_literal_cnt.pop_back();
}
//单子句传播
bool UP(F &f, S_C &sc)
{
    int i
    int signal_clause, num1, num2;
    
    //找到单子句并进行单子句传播
    while ((signal_clause = count(f.clauses_literal_cnt.begin(), f.clauses_literal_cnt.end(), 1)) != 0)
    {
        Find_UP(f, signal_clause, num1, num2);    //找到单子句的位置
        Updatef_sc(f, sc, num1, signal_clause);   //记录更改掉的子句之前的值
        //单子句赋值为真、此文字的计数更新为零
        //进行传播
        f.clause_tf[num1] = true;
        f.literals_cnt[abs(signal_clause) - 1] = 0;
        f.literals_pos[abs(signal_clause) - 1] = signal_clause < 0 ? 0 : 1;
        f.clauses_literal_cnt[num1] = 0;

        //对单子句进行更新
        if(!Simplify_clauses(f, signal_clause)) 
            return false;   //发生冲突
    }
    return 0;
}

void Find_UP(F f, int &signal_clause, int &num1, int &num2)
{
    int num;
    //找到单子句
    for(size_t i=0; i<f.clauses_literal_cnt.size(); i++)
    {
        if(f.clauses_literal_cnt[i] == 1){ num1 = i; f.clauses_literal_cnt[i] = 0; break; }
    }
    for(size_t i=0; i < f.clauses[num1].size(); i++)
    {
        num = f.clauses[num1][i];
        if(f.literals_pos[abs(num) - 1] == -1)
        {
            num2 = i;
            signal_clause = f.clauses[num1][i];
            break;
        }
    }
}

void Simplify_clauses(F &f, int signal_clause, S_C &sc)
{
    //找不为true且包含单子句的子句
    for(size_t i=0; i < f.clause_tf.size(); i++)
    {
        if(!f.clause_tf[i] && f.clauses_literal_cnt[i] > 0)
        {
            int cnt;
            int count = count_if(f.clauses[i].begin(), f.clauses[i].end(), [abs(signal_clause)](int x) {
                return abs(x) == abs(signal_clause);
            });
            if(count > 0)
            {
                Updatef_sc(f, sc, num1, signal_clause);   //记录子句之前的值
                //cnt是返回的子句中的literal的下标，找到句子中包含的所有单文字并赋值
                for(size_t j=0; j<f.clauses[i].size(); j++)
                {
                    if(abs(f.clauses[i][j]) == abs(signal_clause))
                    {
                        if(f.clauses[i][j] < 0)
                        {
                            f.clause_tf[i] = signal_clause < 0 ? true : false;
                            f.clauses_literal_cnt[i] = signal_clause < 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                        }
                        else{
                            f.clause_tf[i] = signal_clause > 0 ? true : false;
                            f.clauses_literal_cnt[i] = signal_clause > 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                        }
                    }
                    if(f.clause_tf[i]) break;   //满足时退出
                    else if(f.clauses_literal_cnt[i] == 0 && f.clause_tf[i] == false)   //出现冲突
                        return false;
                }
            }
        }
    }
}

bool CheckNonClauses(F f)
{
    for(size_t i=0; i<f.clause_tf.size(); i++)
    {
        if(f.clauses_literal_cnt[i] == 0 && f.clause_tf[i] == false) //子句文字都赋值但未满足
        {
            return true;
        }
    }
    return false;
}

bool CheckSatisfy(F f)
{
    for(size_t i=0; i<f.clause_tf.size(); i++)
    {
        if(f.clause_tf[i] != true || f.clauses_literal_cnt[i] != 0) return false;
    }
    return true;
}

void Print(F f, bool satisfy, int64_t t)
{
    if(satisfy)
    {
        printf("s 1\nv ");
        for(size_t i=0; i<f.literals_pos.size(); i++)
        {
            if(f.literals_pos[i] == 0) printf("-%ld ", i+1);
            else if(f.literals_pos[i] == 1) printf("%ld ", i+1);
            else printf("%ld/-%ld ", i+1, i+1);
        }
        printf("\nt %ld\n", t);
    }
    else printf("s -1\n");

    string s = "res/M/" + file + ".res";
    ofstream outfile(s);

    if (satisfy)
    {
        outfile << "s 1" << endl << "v ";
        for (size_t i = 0; i < f.literals_pos.size(); i++)
        {
            if (f.literals_pos[i] == 0)
                outfile << "-" << i + 1 << " ";
            else if (f.literals_pos[i] == 1)
                outfile << i + 1 << " ";
            else
                outfile << i + 1 << "/-" << i + 1 << " ";
        }
        outfile << endl << "t " << t << endl;
    }
    else
    {
        outfile << "s -1" << endl;
    }

    // 关闭输出文件
    outfile.close();
}

void InsertClauses(F &f, int signal_clause)
{
    vector<int> a;  //单子句赋值
    vector<int> new_literal;    //单文字
    
    new_literal.push_back(signal_clause);
    f.clauses.push_back(new_literal);

    a.push_back(-1);

    f.clause_tf.push_back(false);

    f.clauses_literal_cnt.push_back(1);
}