#include "SAT_structure.h"

#include <stack>
#include <chrono>

using namespace std;

//const char* file_name = "D:\\DESKTOP\\SAT\\DPLL\\vs_moms\\vs_moms\\test\\tsmoms\\unsat-php-15-10.cnf";
const char* file_name = "./test/S/problem9-100.cnf";

int main(int argc, char* argv[])
{
    F f;
    vector<Literal> L;
    S_C sc;
    stack<S_C>  save_stack;
    stack<int> flip_v;
    bool flag = false, l = false;
    int i = 0, cnt2, signal_clause = -1;
    flag = false;

    //// 检查是否提供了足够的参数
    //if (argc != 2) {
    //    cerr << "Usage: " << argv[0] << " <file_path>" << endl;
    //    return 1; // 返回非零值表示错误
    //}

    //const char* file_path = argv[1];

    //cout << "File path1: " << file_path << endl;
    //f.F_init(argv, f);

    f.F_init_Debug(f, file_name);
    L.resize(f.all_literal_num + 1);
    f.LiteralInit(f, L);
    f.Prpr_Clause(f, L);    //纯文字化简
    f.SaveChangeInit(sc, f);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    do {
        if (f.UP(f, L, signal_clause, sc))
        {
            signal_clause = f.ChooseLiteral2(f);
            if (signal_clause == -1) { flag = true; break; }
            save_stack.push(sc);
            f.SaveChangeInit(sc, f);     //清空初始化sc
            flip_v.push(signal_clause);
            f.InsertClauses(f, L, signal_clause);
        }
        else
        {
            signal_clause = -1;
            sc.flag = true;
            save_stack.push(sc);
            while (save_stack.top().flip)
            {
                f.BackToPrevious(f, save_stack.top(), L);
                save_stack.pop();
                if (save_stack.empty()) break;
            }
            if (save_stack.top().all_change_c.size() == 0) break;    //找不到满足解
            f.BackToPrevious(f, save_stack.top(), L);
            save_stack.pop();

            if (flip_v.empty()) break;
            //翻转变元
            signal_clause = -flip_v.top();
            f.InsertClauses(f, L, signal_clause);
            flip_v.pop();
            f.SaveChangeInit(sc, f);     //清空初始化sc
            sc.flip = true;     //记录下一个UP∪-x已翻转
        }
    } while (!save_stack.empty());

    if (flag)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        f.Print(f, true, duration);
    }
    else
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        f.Print(f, false, duration);
    }


    //{
    //    stack<F> S_F;
    //    stack<vector<Literal>> S_l;
    //    stack<int> flip_v;
    //    bool flag = false;
    //    int signal_clause, literal, i;
    //    auto start_time = std::chrono::high_resolution_clock::now();
    //    S_F.push(f);
    //    S_l.push(L);
    //    signal_clause = -1;
    //    while (!S_F.empty())
    //    {
    //        f = S_F.top();
    //        L = S_l.top();
    //        while ((i = f.UP(f, L, signal_clause)) == 1)
    //        {
    //            //选取变元加入f
    //            signal_clause = i = f.ChooseLiteral3(f);
    //            if (i == -1) break;
    //            flip_v.push(i);
    //            f.InsertClauses(f, L, flip_v.top());

    //            S_F.push(f);
    //            S_l.push(L);
    //        }
    //        if (i == -1)  //满足
    //        {
    //            signal_clause = -1;
    //            flag = true;
    //            S_F.pop();
    //            S_l.pop();
    //            auto end_time = std::chrono::high_resolution_clock::now();
    //            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    //            f.Print(f, true, duration);
    //            break;
    //        }
    //        if (i == 0)
    //        {
    //            signal_clause = -1;
    //            S_F.pop();
    //            S_l.pop();
    //            if (S_F.empty()) break;    //
    //            f = S_F.top();
    //            L = S_l.top();
    //            S_F.pop();
    //            S_l.pop();
    //            f.InsertClauses(f, L, -(flip_v.top()));
    //            flip_v.pop();
    //            S_F.push(f);
    //            S_l.push(L);
    //        }
    //    }
    //    if (!flag)
    //    {
    //        auto end_time = std::chrono::high_resolution_clock::now();
    //        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    //        f.Print(f, false, duration);
    //    }
    //}
    

    return 0;
}