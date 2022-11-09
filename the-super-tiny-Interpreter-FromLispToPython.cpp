#define _CRT_SECURE_NO_WARNINGS // for strcpy on windows

/*
     LISP 调用语句
        (add 2 (subtract 30 2) )
        |
        |
        |/
      tokenizer()
        |
        |
        |/
        token 数组
            Token* pTokenTab[TOKENNUM];


            ——————————    Token*
pTokenTab->|     - - -| - - - - ->  { type: "paren", pValue: "(" }
            ——————————
           |     - - -| - - - - ->  { pType: "name", pValue: "add" }
            ——————————   Token2
           |    ...   |             { pType: "NumberLiteral", pValue: "2" }
            ——————————
                                    { pType: "paren", pValue: "(" }

                                    { pType: "name", pValue: "subtract" }

                                    { pType: "NumberLiteral", pValue: "30" }

                                    { pType: "NumberLiteral", pValue: "2" }

                                    { pType: "paren", pValue: ")" }

                                    { pType: "paren", pValue: ")" }
*/

#include <iostream>
#include <string>

enum
{
    MAXLEN = 10000,
    TOKENNUM = 5000,
    STRINGLEN = 15,
    SUBNODENUM = 2,
    ASTNUM = 500
};

// testLISPFileStream: 2 个表达式
char gIn[MAXLEN] = "(add 2 (subtract 30 2) )(add 6 3)";

typedef struct Token Token;

// 本版本只支持 左右括号, 函数(操作)名, 数字
struct Token
{
    // Note1: pType = "name"; 实际后续暂时不用, 因为由 token 生成 node 时, 函数名是根据 左括号 token 的 nextToken 确定的
    // Note2: pType = "name" 或 "NumberLiteral" 时, 相应的 pValue 指向 heap => 最后要释放空间
    char* pType;    // "paren" / "name" / "NumberLiteral" / "StringLiteral(暂不支持)"
    char* pValue;   //  "("    / "add" "subtract" / "30" "2" 
};

Token* pTokenTab[TOKENNUM];
int tokenNum = 0;

typedef struct NodeLeaf NodeLeaf;
typedef struct NodeNonLeaf NodeNonLeaf;

struct NodeLeaf
{
    char* pType;    // "NumberLiteral"
    char* pValue;    // "30" "2"
};

struct NodeNonLeaf
{
    char* pType;    // "CallExpression"
    char* pValue;   // "add" / "subtract"

    int   chileNum; // 0 / 1 / 2
    void* pSubNode[SUBNODENUM];  // 解析时, 直接强转为 char* (因为2种 Node 起始内存都存的是 type), 取出 type  
};                               // => 元素是 NodeLeaf*    还是 NodeNonLeaf*, [0/1] 号元素指向 左/右 subNode

void constructToken(int tokenPos, char* pType, char* pValue)
{
    pTokenTab[tokenPos] = (Token*)malloc(sizeof(Token));
    pTokenTab[tokenPos]->pType = pType;
    pTokenTab[tokenPos]->pValue = pValue;
}

void tokenizer()
{
    int curPos = 0;
    int tokenPos = 0;

    long inputLen = strlen(gIn);
    while (curPos < inputLen)
    {
        int ch = gIn[curPos];

        if ('(' == ch)
        {
            constructToken(tokenPos, (char*)"paren", (char*)"(");

            ++curPos;
            ++tokenPos;
            continue;
        }

        if (')' == ch)
        {
            constructToken(tokenPos, (char*)"paren", (char*)")");

            ++curPos;
            ++tokenPos;
            continue;
        }

        // skip whileSpace
        if (' ' == ch || '\t' == ch || '\n' == ch)
        {
            ++curPos;
            continue;
        }

        if (ch - '0' >= 0 && ch - '0' <= 9)
        {
            char* p = (char*)malloc(STRINGLEN * sizeof(char));

            int i = 0;
            while (i < STRINGLEN && ch - '0' >= 0 && ch - '0' <= 9)
            {
                *(p + i) = ch;
                ch = gIn[++curPos];
                ++i;
            }
            if (i < STRINGLEN)
            {
                *(p + i) = '\0';
            }

            constructToken(tokenPos, (char*)"NumberLiteral", p);
            ++tokenPos;

            continue;
        }

        if (ch >= 'a' && ch <= 'z')
        {
            char* p = (char*)malloc(STRINGLEN * sizeof(char));

            int i = 0;
            while (i < STRINGLEN && ch >= 'a' && ch <= 'z')
            {
                *(p + i) = ch;
                ch = gIn[++curPos];
                ++i;
            }
            if (i < STRINGLEN)
            {
                *(p + i) = '\0';
            }

            constructToken(tokenPos, (char*)"name", p);
            ++tokenPos;

            continue;
        }
    }
}

/*

    token 数组
    |
    |
    |/
    parser() - - -> 循序中 递归调 walk(), 每次递归 构建1棵 AST
    |
    |
    |/
    所有 AST
        void* pASTTab[ASTNUM];

            ——————————    void*
pASTTab -> |     - - -| - - - - - - -
            ——————————              |
           |     - - -| - -> AST2   |
            ——————————              |
           |    ...   |             | AST1
            ——————————              |
                                    |/
                            NodeNonLeaf
                                pType =  "CallExpression"
                                pName = "add"
                                chileNum = 2
                                pSubNode[SUBNODENUM];
                                /                   \
                               /                     \
                        pSubNode[0]                 pSubNode[1]
                        /                               \
                       / first subNode: lSubTree         \ second subNode: rSubTree
                      /                                   \
        NodeLeaf                                    NodeNonLeaf
            pType = "NumberLiteral"                     pType =  "CallExpression"
            pValue = "2"                                pValue = "subtract"
                                                        chileNum = 2
                                                        pSubNode[SUBNODENUM];
                                                          /           \
                                                         /             \
                                                        /               \

                                    NodeLeaf                        NodeLeaf
                                        pType = "NumberLiteral"         pType = "NumberLiteral"
                                        pValue = "30"                   pValue = "2"
*/

void* pASTTab[ASTNUM];
int astNum = 0;

void* createNode(char* pType, char* pValue);

NodeLeaf*
createNodeLeaf(char* pType, char* pValue)
{
    NodeLeaf* p = (NodeLeaf*)malloc(sizeof(NodeLeaf));

    p->pType = pType;
    p->pValue = pValue;

    return p;
}

NodeNonLeaf*
createNodeNonLeaf(char* pType, char* pValue)
{
    NodeNonLeaf* p = (NodeNonLeaf*)malloc(sizeof(NodeNonLeaf));

    p->pType = pType;
    p->pValue = pValue;
    p->chileNum = 0;
    p->pSubNode[0] = p->pSubNode[1] = NULL;

    return p;
}

void* createNode(char* pType, char* pValue)
{
    if (0 == strcmp("NumberLiteral", (const char*)pType))
    {
        return createNodeLeaf(pType, pValue);
    }
    else if (0 == strcmp("CallExpression", pType) ||
        0 == strcmp("ExpressionStatement", pType) )
    {
        return createNodeNonLeaf(pType, pValue);
    }
    else
    {
        return NULL;
    }
}

int curTokenPos = 0;

void* walk()
{
    Token* pToken = pTokenTab[curTokenPos];

    if (0 == strcmp("NumberLiteral", (const char*)pToken->pType))
    {
        ++curTokenPos;

        return createNode(pToken->pType, pToken->pValue);
    }
    else if (0 == strcmp("paren", (const char*)pToken->pType) &&
        0 == strcmp("(", (const char*)pToken->pValue))
    {
        // skip "("
        pToken = pTokenTab[++curTokenPos];

        // "("  的 nextToke : 必为 CallExpression
        NodeNonLeaf* pNodeNonLeaf = (NodeNonLeaf*)createNode((char*)"CallExpression", pToken->pValue);

        //Note: "CallExpression" token 后的 最多2个子节点, while 最多循环2次, 
        // 每次迭代 递归调 walk() 更新 token 的全局遍历指针 curTokenPos
        // firstsSubNode 
        pToken = pTokenTab[++curTokenPos];
        while (0 != strcmp("paren", (const char*)pToken->pType) ||
            0 == strcmp("paren", (const char*)pToken->pType) && 0 != strcmp(")", (const char*)pToken->pValue))
        {
            pNodeNonLeaf->pSubNode[pNodeNonLeaf->chileNum++] = walk();

            // Note: global curTokenPos 已在 下层 walk() 调用中 更新
            pToken = pTokenTab[curTokenPos];
        }

        // 必然 "paren" == pToken->pType && 0 == strcmp(pToken->pValue, ")")
        // skip 本 pNodeNonLeaf 匹配的 ")"
        ++curTokenPos;

        return pNodeNonLeaf;
    }
    else
    {
        return NULL;
    }
}

void calcTokenNum()
{
    while (NULL != pTokenTab[tokenNum])
    {
        ++tokenNum;
    }
}

void parser()
{
    calcTokenNum();

    // 每次循环处理1个 AST, walk 内部更新 global tokenNum
    while (curTokenPos < tokenNum)
    {
        pASTTab[astNum++] = walk();
    }
}

/*
    oldAST
     |
     |
     |/
    transformer
    |
    |
    |/
    new AST 
        

如: 若AST 对应的 LISP 语句是 表达式语句 => rootNode.type == "CallExpression"
给 rootNode 增加1个 parentNode, rootNode 作其左孩子

所有 AST
        void* pNewASTTab[ASTNUM];

            ——————————    void*
           |	 - - -| - - - - - - -
            ——————————			    |
           |	 - - -| - -> AST2	|
            ——————————				|
           |	...	  |				| AST1
            ——————————				|
                                    |/
                            NodeNonLeaf
                                pType =  "ExpressionStatement" // 区分 纯表达式语句, 如 (add 2 3) 和 表达式后赋值等语句, 如 (set var (add 2 3)), 暂时只支持第1种
                                pName = ""   // 空
                                chileNum = 1 // 1个左孩子
                                pSubNode[SUBNODENUM];
                                     /                \
                                    /			       \
                                   /		            \
                                  / 					 \
                            NodeNonLeaf					   NULL
                                pType =  "CallExpression"
                                pName = "add"
                                chileNum = 2
                                pSubNode[SUBNODENUM];
                                /					\
                               /					 \
                        pSubNode[0]					pSubNode[1]
                        / 								\
                       / first subNode: lSubTree		 \ second subNode: rSubTree
                      /									  \
        NodeLeaf									NodeNonLeaf
            pType = "NumberLiteral"           			pType =  "CallExpression"
            pValue = "2"			        			pValue = "subtract"
                                                        chileNum = 2
                                                        pSubNode[SUBNODENUM];
                                                          /			  \
                                                         / 			   \
                                                        /			 	\

                                    NodeLeaf						NodeLeaf
                                        pType = "NumberLiteral"     	pType = "NumberLiteral"
                                        pValue = "30"			        pValue = "2"
*/

/*
用访问者模式 操作 AST 的每个 Node 
    
    ———————————        ———————————
   |     Node            |       |  Visitor             |
    ———————————        ———————————
   | accept(Visitor& v)  |       | accept(Node&)        |
    ——|————————        —|——————————
        |                          |\
        |                          |
      v.accept(*this) - - - - - - - 

        Visitor::accept(Node& node)
            据 nodeType 的不同 对 node 执行不同操作

*/

void* pNewASTTab[ASTNUM];

struct Visitor // unused now 
{
    void accept(NodeLeaf* node, NodeLeaf* parent)
    {
        
    }
};

void* 
traverseNode(struct Visitor* pVisitor, NodeLeaf* pNode, NodeNonLeaf* parent)
{
    if (0 == strcmp("NumberLiteral", pNode->pType))
    {
        // Note 新旧 AST 的成员指针指向相同的内存
        NodeLeaf* newNode = (NodeLeaf*)createNode(pNode->pType, pNode->pValue);

        return newNode;
    }
    else if (0 == strcmp("CallExpression", pNode->pType))
    {
        if (NULL == parent) // rootCallExpression
        {
            NodeNonLeaf* pNewAst = (NodeNonLeaf*)createNode((char*)"ExpressionStatement", (char*)"ES");
            NodeNonLeaf* pNewNode = (NodeNonLeaf*)createNode(pNode->pType, pNode->pValue);
            pNewAst->pSubNode[pNewAst->chileNum++] = pNewNode; // chileNum = 1;


            NodeNonLeaf* pNodeMid = (NodeNonLeaf*)pNode;
            while(pNewNode->chileNum < pNodeMid->chileNum)
            {
                // i = 0 / 1 -> 左 / 右
                pNewNode->pSubNode[pNewNode->chileNum] = 
                    traverseNode(pVisitor, (NodeLeaf*)pNodeMid->pSubNode[pNewNode->chileNum], (NodeNonLeaf*)pNode);

                ++pNewNode->chileNum;
            }

            return pNewAst; // EXIT: 顶级出口
        }
        else // nonRootCallExpression
        {
            NodeNonLeaf* pNewNode = (NodeNonLeaf*)createNode(pNode->pType, pNode->pValue);

            NodeNonLeaf* pNodeMid = (NodeNonLeaf*)pNode;
            while(pNewNode->chileNum < pNodeMid->chileNum)
            {
                // i = 0 / 1 -> 左 / 右
                pNewNode->pSubNode[pNewNode->chileNum] = 
                    traverseNode(pVisitor, (NodeLeaf*)pNodeMid->pSubNode[pNewNode->chileNum], (NodeNonLeaf*)pNode);
                
                ++pNewNode->chileNum;
            }

            return pNewNode;
        }
    }
    else
    {
        return NULL;
    }
}

NodeNonLeaf* 
traverserASTAndCreateNewAST(struct Visitor* pVisitor, void* pAST)
{
    return (NodeNonLeaf*)traverseNode(pVisitor, (NodeLeaf*)pAST, (NodeNonLeaf*)NULL);
}

void transformer() 
{
    Visitor visitor;

    // 每次循环处理1个 AST, walk 内部更新 global tokenNum
    for(int astIndex = 0; astIndex < astNum; ++astIndex)
    {
        pNewASTTab[astIndex] = (void*)traverserASTAndCreateNewAST(&visitor, pASTTab[astIndex] );
    } 
}

/*
    所有 AST
        |
        |
        |/
    codeGenerator(): 循环中递归调 codeGeneratorOneAst(pASTTab[astIndex++] ) 每次递归 生成1个语句
        |
        |
        |/
    C (调用)语句
        add(2, subtract(30, 2));
*/

// === test: 符合预期

// gOut = 0x01063618 "add(2, subtract(30, 2));\nadd(6, 3);\n"

char gOut[MAXLEN];

char outSreamCurPos = 0;

// mapReduce
// void myMap(void* pSubNode[], int childNum, char* op(void* pAst) ){ }

void codeGeneratorOneAst(void* pAst)
{
    NodeLeaf* pNode = (NodeLeaf*)pAst;
    
    if (0 == strcmp("NumberLiteral", pNode->pType))
    {
        int valueStrLen = strlen(pNode->pValue);
        strcpy(gOut + outSreamCurPos, pNode->pValue);
        outSreamCurPos += valueStrLen; // 覆盖 拷贝完成后尾部的 '\0'

        // std::copy(pNode->pValue, pNode->pValue + valueStrLen, gOut);
    }
    else if (0 == strcmp("CallExpression", pNode->pType))
    {
        NodeNonLeaf* pNodeNonLeaf = (NodeNonLeaf*)pNode;

        int valueStrLen = strlen(pNodeNonLeaf->pValue);
        strcpy(gOut + outSreamCurPos, pNodeNonLeaf->pValue);

        outSreamCurPos += valueStrLen;
        gOut[outSreamCurPos++] = '('; // 跳到下1个 字符起始

        // return myMap(pNodeNonLeaf->pSubNode, pNodeNonLeaf->chileNum, codeGeneratorOneAst);

        // childNum == 1 / 2: 先序遍历: 根 左( pSubNode[0] ) 右( pSubNode[1] )
        for (int i = 0; i < pNodeNonLeaf->chileNum; ++i)
        {
            NodeLeaf* pSubNode = (NodeLeaf*)pNodeNonLeaf->pSubNode[i];

            if (0 == strcmp("NumberLiteral", (const char*)pSubNode->pType))
            {
                codeGeneratorOneAst(pSubNode);
            }
            else if (0 == strcmp("CallExpression", (const char*)pNode->pType))
            {
                NodeNonLeaf* pSubNodeNonLeaf = (NodeNonLeaf*)pNodeNonLeaf->pSubNode[i];
                codeGeneratorOneAst(pSubNodeNonLeaf);
            }
            else
            {
                //
            }

            if (0 == i) // 左孩子处理完, 用 "," 隔开
            {
                gOut[outSreamCurPos++] = ',';
                gOut[outSreamCurPos++] = ' ';
            }
            else if (1 == i)
            {
                gOut[outSreamCurPos++] = ')';
            }
        }
    }
    else if (0 == strcmp("ExpressionStatement", pNode->pType))
    {
        codeGeneratorOneAst( ( (NodeNonLeaf*)pNode)->pSubNode[0] );
    }
    else // "Program" / "ExpressionStatement"
    {
        // 
    }
}

void codeGenerator()
{
    int astIndex = 0;
    while (astIndex < astNum)
    {
        codeGeneratorOneAst(pNewASTTab[astIndex++]);

        gOut[outSreamCurPos++] = ';';
        gOut[outSreamCurPos++] = '\n';
    }
}

void preTraverserPrintValue(void* pNode)
{
    if (0 == strcmp("NumberLiteral", (const char*)((NodeLeaf*)pNode)->pType))
    {
        puts(((NodeLeaf*)pNode)->pValue);
    }
    else if (0 == strcmp("CallExpression", (const char*)((NodeLeaf*)pNode)->pType) ||
            0 == strcmp("ExpressionStatement", (const char*)((NodeLeaf*)pNode)->pType)
           )
    {
        puts(((NodeLeaf*)pNode)->pValue);

        for (int i = 0; i < ((NodeNonLeaf*)pNode)->chileNum; ++i)
        {
            preTraverserPrintValue(((NodeNonLeaf*)pNode)->pSubNode[i]);
        }    
    }
    else
    {
        // 
    }
}

void test1()
{
    NodeNonLeaf* pAst = NULL;
    for (int i = 0; i < astNum; ++i)
    {
        pAst = (NodeNonLeaf*)pASTTab[i];

        preTraverserPrintValue(pAst);
    }
}

void test2()
{
    std::cout << "----------\n\n";
    NodeNonLeaf* pAst = NULL;
    for (int i = 0; i < astNum; ++i)
    {
        pAst = (NodeNonLeaf*)pNewASTTab[i];

        preTraverserPrintValue(pAst);
    }
}

int main()
{
    tokenizer();

    parser();

    test1();

    transformer();

    test2();

    codeGenerator();
}
