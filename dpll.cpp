#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <chrono>
#include <algorithm>

using namespace std;

//输入算例
class F
{
    public:
    vector<int> literals_pos;   //文字 0-false 1-true -1-undesign -2-未出现
    vector<int> literals_cnt;   //文字出现次数
    vector<int> clauses_literal_cnt;    //子句长度
    vector<bool> clause_tf;     //子句是否满足
    vector<vector<int>> clauses;    //子句
    // vector<vector<int>> literal_clause;     //文字所在子句
    vector<vector<int>> clauses_sta;    //子句赋值情况->可以优化？

    F(){}

    F(const F &f)
    {
        literals_pos = f.literals_pos;
        clauses = f.clauses;
        literals_cnt = f.literals_cnt;
        clauses_sta = f.clauses_sta;
        clauses_literal_cnt = f.clauses_literal_cnt;
        clause_tf = f.clause_tf;
    }
};

string file = "unsat-5cnf-30";
bool flag = false;
int all_literals, all_clauses;

void init(F &f);
bool CheckNonClauses(F f);
bool CheckSatisfy(F f);
int ChooseLiteral(F f);
int FindSignalLiteral(F f, int signal_clause, int index_of_clause);
void UP(F &f); 
bool DPLL(F &f);
void Print(F f, bool satisfy, int64_t t);

int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    
    F f;
    init(f);
    DPLL(f);
    if(!flag) printf("s -1\n");
    else
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        Print(f, true, duration);
    }
    return 0;
}

void init(F &f)
{
    string s = "test/" + file + ".cnf";
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
            f.clauses_sta.clear();
            f.clauses_sta.resize(all_clauses);      // <- 对应clauses中文字的赋值
            f.clauses_literal_cnt.clear();
            f.clauses_literal_cnt.resize(all_clauses, 0);
            f.clause_tf.clear();
            f.clause_tf.resize(all_clauses, false);
            continue;
        }

        while(iss >> literal)
        {
            if(literal == 0) {
                f.clauses_literal_cnt[i] = j;   //子句中文字数量
                f.clauses_sta[i++].resize(j, -1);   //把每个子句中文字的赋值初始化
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

int FindSignalLiteral(F f, int signal_clause, int index_of_clause)
{
    for(size_t i=0; i<f.clauses[index_of_clause].size(); i++)
    {
        if(abs(f.clauses[index_of_clause][i]) == abs(signal_clause)) return i;
    }
    return -1;
}

int ChooseLiteral(F f)
{
    //返回最多次文字出现的下标
    return max_element(f.literals_cnt.begin(), f.literals_cnt.end()) - f.literals_cnt.begin() + 1;
}

///单子句传播
void UP(F &f)
{
    int signal_clause, num1, num2;
    queue<int> choose_literal;
    //检查是否有空子句
    if(CheckNonClauses(f)) return ;
    //找到单子句并进行单子句传播
    //优化：用数组记录每个子句的长度
    while ((signal_clause = count(f.clauses_literal_cnt.begin(), f.clauses_literal_cnt.end(), 1)) != 0)
    {
        //找到单子句
        for(size_t i=0; i<f.clauses_literal_cnt.size(); i++)
        {
            if(f.clauses_literal_cnt[i] == 1){ num1 = i; f.clauses_literal_cnt[i] = 0; break; }
        }
        for(size_t i=0; i < f.clauses_sta[num1].size(); i++)
        {
            if(f.clauses_sta[num1][i] == -1)
            {
                num2 = i;
                signal_clause = f.clauses[num1][i];
                f.clauses_literal_cnt[num1] = 0;
                break;
            }
        }

        //单子句赋值为真、此文字的计数更新为零
        //进行传播
        //正负极性弄得太麻烦了，如何化简->直接不设置正负极性?
        f.clause_tf[num1] = true;
        f.literals_cnt[abs(signal_clause) - 1] = 0;
        f.literals_pos[abs(signal_clause) - 1] = signal_clause < 0 ? 0 : 1;
        f.clauses_sta[num1][num2] = 1;  //单子句为真
        
        //找不为true且包含单子句的子句
        for(size_t i=0; i < f.clause_tf.size(); i++)
        {
            if(!f.clause_tf[i])
            {
                int cnt;
                //cnt是返回的子句中的literal的下标，找到句子中包含的所有单文字并赋值
                for(size_t j=0; j<f.clauses[i].size(); j++)
                {
                    if(abs(f.clauses[i][j]) == abs(signal_clause))
                    {
                        if(f.clauses[i][j] < 0)
                        {
                            f.clauses_sta[i][j] = signal_clause < 0 ? 1 : 0;
                            f.clause_tf[i] = signal_clause < 0 ? true : false;
                            f.clauses_literal_cnt[i] = signal_clause < 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                        }
                        else{
                            f.clauses_sta[i][j] = signal_clause > 0 ? 1 : 0;
                            f.clause_tf[i] = signal_clause > 0 ? true : false;
                            f.clauses_literal_cnt[i] = signal_clause > 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                        }
                    }
                }

                // if((cnt = FindSignalLiteral(f, signal_clause, i)) != -1)
                // {
                //     if(f.clauses[i][cnt] < 0)
                //     {
                //         f.clauses_sta[i][cnt] = signal_clause < 0 ? 1 : 0;
                //         f.clause_tf[i] = signal_clause < 0 ? true : false;
                //         f.clauses_literal_cnt[i] = signal_clause < 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                //     }
                //     else{
                //         f.clauses_sta[i][cnt] = signal_clause > 0 ? 1 : 0;
                //         f.clause_tf[i] = signal_clause > 0 ? true : false;
                //         f.clauses_literal_cnt[i] = signal_clause > 0 ? 0 : f.clauses_literal_cnt[i] - 1;
                //     }
                // }
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

bool DPLL(F &f)
{
    //函数出口怎么找
    // if(CheckSatisfy(f)) return true;    //?????
    // if(CheckNonClauses(f)) return false;


    UP(f);

    if(CheckSatisfy(f)) 
        return true;
    if(CheckNonClauses(f)) 
        return false;

    int literal = ChooseLiteral(f);
    vector<int> a;  //单子句赋值
    vector<int> new_literal;    //单文字
    
    F f_clone = f;
    new_literal.push_back(literal);
    f.clauses.push_back(new_literal);
    a.push_back(-1);
    f.clauses_sta.push_back(a);
    f.clause_tf.push_back(false);
    f.clauses_literal_cnt.push_back(1);

    bool test = DPLL(f);
    if(test)
    {
        flag = true;
        return true;
    }
    else
    {
        f = f_clone;
        new_literal.pop_back();
        new_literal.push_back(-literal);

        f.clauses.push_back(new_literal);
        f.clauses_sta.push_back(a);
        f.clause_tf.push_back(false);
        f.clauses_literal_cnt.push_back(1);

        test = DPLL(f);
        if(test) return true;
        else return false;
    }
}

void Print(F f, bool satisfy, int64_t t)
{
    if(satisfy)
    {
        printf("s 1\nv ");
        for(size_t i=0; i<f.literals_pos.size(); i++)
        {
            if(f.literals_pos[i] == 0) printf("-%d ", i+1);
            else if(f.literals_pos[i] == 1) printf("%d ", i+1);
            else printf("%d/-%d ", i+1, i+1);
        }
        printf("\nt %ld\n", t);
    }
    else printf("s -1\n");

    string s = "res/" + file + ".res";
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
    f.clauses_sta.push_back(a);

    f.clause_tf.push_back(false);

    f.clauses_literal_cnt.push_back(1);
}