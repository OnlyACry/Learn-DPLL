#include <iostream>

using namespace std;

/// @brief 每个结点
typedef struct DataNode{
    int value = 0;
    struct DataNode* next;
}DataNode;

/// @brief 头节点连接子句，并表示子句状态
typedef struct HeadNode{
    int num = 0;
    struct HeadNode* down;
    struct DataNode* right;
}HeadNode;

int ChooseSignalLiteral(HeadNode* F);

int UP(HeadNode* F, int* all_literal);

void Simplify_clauses(HeadNode* F, int signal_literal);

bool CheckNonClause(HeadNode* F);

HeadNode* Merge_x(HeadNode* F, int x);

int ChooseLiteral(HeadNode* F);

HeadNode* Copy_F(HeadNode* F);

void Free_Space_F(HeadNode* F);


/// @brief 选择单子句
/// @param F 
/// @return 单子句或无
int ChooseSignalLiteral(HeadNode* F)
{

}

/// @brief 单子句传播
/// @param F 子句集合
/// @param all_literal 文字集合
/// @return 单子句传播结果->冲突 or 满足
int UP(HeadNode* F, int* all_literal)
{

}

/// @brief 根据单子句signal_literal化简F，去掉句子中所有的这个单子句
/// @param F 
/// @param signal_literal 
void Simplify_clauses(HeadNode* F, int signal_literal)
{

}

/// @brief 检查是否有空子句
/// @param F 
/// @return 
bool CheckNonClause(HeadNode* F)
{

}

/// @brief 将变元x添加到F中
/// @param F 
/// @param x 
/// @return 
HeadNode* Merge_x(HeadNode* F, int x)
{

}

/// @brief 选择变元
/// @param F 
/// @return 
int ChooseLiteral(HeadNode* F)
{

}

/// @brief 复制F
/// @param F 
/// @return 
HeadNode* Copy_F(HeadNode* F)
{
    HeadNode* _cf = new HeadNode;
    _cf->num = F->num;

    HeadNode* p = F->down;
    HeadNode* _cf_p = new HeadNode;
    _cf->down = _cf_p;
    _cf_p->down = nullptr;

    for(int i=0; i<F->num; i++)
    {

    }
}

/// @brief 释放空间
/// @param F 
void Free_Space_F(HeadNode* F)
{
    HeadNode* p = F->down;
    while(p){
        DataNode* q = p->right;
        while(q){
            DataNode* _fq = q;
            q = q->next; 
            delete _fq;
        }
        HeadNode* _fp = p;
        p = p->down;
        delete _fp;
    }
    delete F;
}