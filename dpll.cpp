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
    vector<int> literals_pos;   //文字 0-false 1-true -1-undesign
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

string file = "tst_v10_c100";
bool flag = false;
int all_literals, all_clauses;

void init(F &f);
bool CheckNonClauses(F f);
bool CheckSatisfy(F f);
int ChooseLiteral(F f);
int ChooseLiteral2(F f);
int FindSignalLiteral(F f, int signal_clause, int index_of_clause);
bool UP(F &f); 
void Find_UP(F f, int& signal_clause, int &num1, int &num2);
bool Simplify_clauses(F &f, int signal_clause);
void InsertClauses(F &f, int signal_clause);    //插入单子句
bool DPLL(F &f);
void Print(F f, bool satisfy, int64_t t);
void Check(F f);

int main() 
{
    double cnt = 0;
    // for (int iteration = 1; iteration <= 5; ++iteration) 
    // {
        F f;
        init(f);
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if(DPLL(f))
        {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            Print(f, true, duration);
            cnt += duration;
        }
        else
        {
            printf("s -1");
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            printf("\nt %ld\n", duration);
            cnt += duration;
        }
    // }
    printf("%f", cnt/5000.0);
    return 0;
}
void init(F &f)
{
    string s = "test/tst/" + file + ".cnf";
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
            f.clauses.resize(all_clauses, vector<int>());
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
            f.literals_cnt[abs(literal) - 1]++;
            f.clauses[i].push_back(literal);
            j++;
        }
    }

    // i = 0;
    // string satinput = "-1 2 -3 4 -5 -6 7 8 -9 10 11 -12 -13 -14 15 -16 -17 18 -19 20 -21 -22 -23 24 -25 -26 27 -28 -29 -30 -31 32 -33 -34 35 36 -37 -38 -39 -40 41 42 43 44 45 -46 -47 -48 49 -50 -51 52 -53 54 55 -56 57 58 59 60 61 -62 63 -64 -65 66 -67 -68 -69 -70 71 72 73 74 75 76 -77 78 -79 -80 -81 -82 -83 -84 85 86 -87 -88 -89 -90 91 92 -93 -94 -95 -96 97 -98 99 -100";
    // istringstream is(satinput);
    // while(is >> literal)
    // {
    //     if(literal < 0) f.literals_pos[i++] = 0;
    //     else f.literals_pos[i++] = 1;
    // }
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
    //返回最多次文字
    return max_element(f.literals_cnt.begin(), f.literals_cnt.end()) - f.literals_cnt.begin() + 1;
}

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

///单子句传播
bool UP(F &f)
{
    int signal_clause, num1, num2;
    //找到单子句并进行单子句传播
    while ((signal_clause = count(f.clauses_literal_cnt.begin(), f.clauses_literal_cnt.end(), 1)) != 0)
    {
        Find_UP(f, signal_clause, num1, num2);    //找到单子句的位置

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
    return true;
}

void Find_UP(F f, int &signal_clause, int &num1, int &num2)
{
    int num;
    //找到单子句
    for(size_t i=0; i<f.clauses_literal_cnt.size(); i++)
    {
        if(f.clauses_literal_cnt[i] == 1)
        { 
            num1 = i; break; 
        }
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

bool Simplify_clauses(F &f, int signal_clause)
{
    int num;
    //找不为true且包含单子句的子句
    for(size_t i=0; i < f.clause_tf.size(); i++)
    {
        if(!f.clause_tf[i] && f.clauses_literal_cnt[i] > 0)
        {
            for(size_t j=0; j<f.clauses[i].size(); j++)
            {
                num = abs(f.clauses[i][j]);
                if(num == abs(signal_clause))
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
    return true;
}

void InsertClauses(F &f, int signal_clause)
{
    vector<int> a;  //单子句赋值
    vector<int> new_literal;    //单文字
    
    new_literal.push_back(signal_clause);
    f.clauses.push_back(new_literal);

    a.push_back(-1);
    // f.clauses_sta.push_back(a);

    f.clause_tf.push_back(false);

    f.clauses_literal_cnt.push_back(1);
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
    if(!UP(f)) return false;

    // int literal = ChooseLiteral(f);
    int literal = ChooseLiteral2(f);

    if(literal == -1) return true;

    F f_clone = f;
    InsertClauses(f, literal);
    if(DPLL(f)) return true;
    else{
        f = f_clone;
        InsertClauses(f, -literal);
        return DPLL(f);
    }
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

    string s = "res/tst/tst/recursion1/" + file + ".res";
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