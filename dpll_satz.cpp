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
    bool flip = false;
    bool flag = false;
    set<int> change_v;
    vector<int> v_cnt;      //记录改变的文字，其赋值为此文字原本在整个算例中出现的次数
    vector<int> v_pos;      //记录改变的文字，其赋值为此文字原本在算例的值
    map<int, struct Save_C> all_change_c;   //记录修改之前的句子

    S_C(){}

    S_C(const S_C &sc)
    {
        v_cnt = sc.v_cnt;
        all_change_c = sc.all_change_c;
        change_v = sc.change_v;
        flip = sc.flip;
        flag = sc.flag;
        v_pos = sc.v_pos;
    }
};

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
        literals_cnt = f.literals_cnt;
        clauses_literal_cnt = f.clauses_literal_cnt;
        clause_tf = f.clause_tf;

        // 深拷贝 clauses
        clauses.clear();
        clauses.reserve(f.clauses.size());
        for (const auto &clause : f.clauses) {
            clauses.push_back(clause);  // 这里会调用 vector<int> 的拷贝构造函数来创建新的副本
        }
    }
};

string file = "4blocksb";
int all_literals, all_clauses;

void init(F &f, S_C &sc);
void initsc(S_C &sc);
bool CheckNonClauses(F f);
bool CheckSatisfy(F f);
int ChooseLiteral(F f);
int ChooseLiteral2(F f);
bool UP(F &f, S_C &sc); 
void Find_UP(F f, int& signal_clause, int &num1, int &num2);
void Print(F f, bool satisfy, int64_t t);
void InsertClauses(F &f, int signal_clause);    //插入单子句
bool Simplify_clauses(F &f, int signal_clause, S_C &sc);
void BackToPrevious(F &f, S_C sc);
void Updatef_sc(F &f, S_C &sc, int i, int signal_clause);
void Check(F f);

int main()
{
    S_C sc;
    stack<S_C>  save_stack;
    stack<int> flip_v;
    F f, f_clone;
    double cnt = 0;
    bool flag = false, l = false;
    int i = 0, signal_clause;
    
    init(f, sc);
    auto start_time = std::chrono::high_resolution_clock::now();
    do{
        if(UP(f, sc))
        {
            signal_clause = ChooseLiteral2(f);
            if(signal_clause == -1) {flag = true; break;}
            save_stack.push(sc);
            initsc(sc);     //清空初始化sc
            flip_v.push(signal_clause);
            InsertClauses(f, signal_clause);
        }
        else
        {
            sc.flag = true;
            save_stack.push(sc);
            while (save_stack.top().flip)
            {
                BackToPrevious(f, save_stack.top());
                save_stack.pop();
                if(save_stack.empty()) break;
            }
            if(save_stack.top().all_change_c.size() == 0) break;    //找不到满足解
            BackToPrevious(f, save_stack.top());
            save_stack.pop();
            
            if(flip_v.empty()) break;
            //翻转变元
            signal_clause = flip_v.top();
            InsertClauses(f, -signal_clause);
            flip_v.pop();
            initsc(sc);     //清空初始化sc
            // sc.flag = true;
            sc.flip = true;     //记录下一个UP∪-x已翻转
        }
    }while(!save_stack.empty());

    if(flag)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        Print(f, true, duration);
        cnt += duration;
        // Check(f);
    }
    else
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        printf("s -1");
        printf("\nt %ld\n", duration);
        cnt += duration;
        // Check(f);
    }
    return 0;
}

void initsc(S_C &sc)
{
    sc.all_change_c.clear();
    sc.change_v.clear();
    sc.v_cnt.clear();
    sc.v_cnt.resize(all_literals, -1);
    sc.v_pos.clear();
    sc.v_pos.resize(all_literals, -1);  //插入单子句的变化怎么办
    sc.flag = false;
    sc.flip = false;
}

void init(F &f, S_C &sc)
{
    string s = "test/Beijing/" + file + ".cnf";
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
            sc.v_cnt.resize(all_literals, -1);
            sc.v_pos.resize(all_literals, -1);
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
    bool flag = false;
    int min = INT16_MAX, index, num;
    for(int i=0; i<f.clauses.size(); i++)
    {
        if(f.clause_tf[i] == true || f.clauses_literal_cnt[i] == 0) continue;
        if(f.clauses_literal_cnt[i] > 0 && f.clauses_literal_cnt[i] < min)
        {
            flag = true;
            min = f.clauses_literal_cnt[i];
            index = i;
        }
    }
    if(!flag)   //没找到单子句，证明已经满足
    {
        return -1;
    }
    int max = INT16_MIN, n = -1, t;
    for(int j=0; j<f.clauses[index].size(); j++)
    {
        n = f.clauses[index][j];
        t = f.literals_pos[abs(n) - 1];
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
    int num = abs(signal_clause) - 1;
    struct Save_C pov_clause;
    //首先检测是否已有键值
    if(sc.all_change_c.find(i) != sc.all_change_c.end()) {} //存在映射中，不更新
    else
    {
        pov_clause.length = f.clauses_literal_cnt[i];//句子长度
        pov_clause.tf = f.clause_tf[i];             //句子满足性
        sc.all_change_c[i] = pov_clause;
    }
    if(sc.change_v.find(num) != sc.change_v.end()) {}     //文字之前已经保存过值，不能覆盖
    else
    {
        sc.v_cnt[num] = f.clauses_literal_cnt[num]; //文字总数
        sc.v_pos[num] = f.literals_pos[num];        //文字改变前
        sc.change_v.insert(num);
    }
}

void BackToPrevious(F &f, S_C sc)
{
    int index;
    struct Save_C back_c;
    if(sc.all_change_c.empty()) return;
    for (map<int, struct Save_C>::iterator it = sc.all_change_c.begin(); it != sc.all_change_c.end(); ++it) 
    {
        index = it->first;
        back_c = it->second;
        f.clause_tf[index] = back_c.tf;
        f.clauses_literal_cnt[index] = back_c.length;
        // f.literals_cnt[index] = sc.v[index];
    }
    for (const int& element : sc.change_v) {
        f.literals_cnt[element] = sc.v_cnt[element];
        f.literals_pos[element] = sc.v_pos[element];
    }
    f.clauses.pop_back();
    f.clause_tf.pop_back();
    f.clauses_literal_cnt.pop_back();
}
//单子句传播
bool UP(F &f, S_C &sc)
{
    int i;
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
        if(!Simplify_clauses(f, signal_clause, sc)) 
            return false;   //发生冲突
    }
    return true;
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

bool Simplify_clauses(F &f, int signal_clause, S_C &sc)
{
    //找不为true且包含单子句的子句
    for(size_t i=0; i < f.clause_tf.size(); i++)
    {
        if(!f.clause_tf[i] && f.clauses_literal_cnt[i] > 0)
        {
            int n = abs(signal_clause);
            int count = count_if(f.clauses[i].begin(), f.clauses[i].end(), [n](int x) {
                return abs(x) == n;
            });
            if(count > 0)
            {
                Updatef_sc(f, sc, i, signal_clause);   //记录子句之前的值
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
    return true;
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
        // for(size_t i=0; i<f.literals_pos.size(); i++)
        // {
        //     if(f.literals_pos[i] == 0) printf("-%ld ", i+1);
        //     else if(f.literals_pos[i] == 1) printf("%ld ", i+1);
        //     else printf("%ld/-%ld ", i+1, i+1);
        // }
        printf("\nt %ld\n", t);
    }
    else printf("s -1\n");

    string s = "res/tst/tst/better/" + file + ".res";
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

void Check(F f)
{
    F new_f;
    int choose;
    bool fl = false;
    new_f.clauses = f.clauses;
    new_f.literals_pos = f.literals_pos;

    for(size_t i=0; i<new_f.clauses.size(); i++)
    {
        fl = false;
        for(size_t j=0; j<new_f.clauses[i].size(); j++)
        {
            choose = new_f.literals_pos[abs(new_f.clauses[i][j]) - 1];
            if(choose == -1)
            {
                fl=true;
                break;
            }
            else
            {
                if(new_f.clauses[i][j] < 0 && choose == 0)
                {
                    fl = true;
                    break;
                }
                if(new_f.clauses[i][j] > 0 && choose == 1)
                {
                    fl = true;
                    break;
                }
            }
        }
    }
    if(fl)
    {
        printf("\nyes\n");
        return;
    }
    else printf("\nno\n");
}