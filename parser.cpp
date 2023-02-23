/**
 * Auther: Wei Ge
 * Date: Jan 23, 2023
 * Parser: Input token, output the AST
 */

#include <stdio.h>
#include <string>
#include <memory>
#include <vector>

#include "lexer.h"
#include "parser.h"

/**************************** Build the AST strcuture ****************************/

// the base class for expression nodes
class ExprAST
{
public:
    virtual ~ExprAST() {}
};

// the expression for numeric literals
class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    Value* codegen() override;
};

// the expression for referencing a variable
class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
};

// the expression for binary operator
class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS) : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

// the expression for function calls
class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST> > Args;

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST> > Args)
        : Callee(Callee), Args(std::move(Args)) {} // FIXME:Args(std::move(Args) error
};

class PrototypeAST
{
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> Args)
        : Name(name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

/* Parse the token to AST */

// parse"x+y"的token的例子
// auto LHS = std::make_unique<VariableExprAST>("x");
// auto RHS = std::make_unique<VariableExprAST>("y");
// auto Result = std::make_unique<BinaryExprAST>('+', std::move(LHS), std::move(RHS));

// read token one by one
// store the token which will be transfered into CurTok
static int CurTok;
int getNextToken()
{
    return CurTok = gettok();
}

// log error
std::unique_ptr<ExprAST> LogError(const char *Str)
{
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
    LogError(Str);
    return nullptr;
}

/**************************** Parser Driver ****************************/
void MainParser()
{
    while (true)
    {
        fprintf(stderr, "ready> ");
        switch (CurTok)
        {
            case tok_eof:
                return;
            // 忽略;
            case ';':
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

/**************************** Basic Expression parse ****************************/

// parse expressions
// 所有Expression = basic expression + binary operator
static std::unique_ptr<ExprAST> ParseExpression()
{
    // 首先parse LHS basic expression
    auto LHS = ParseBasicExpression();
    if (!LHS)
        return nullptr;

    // 然后处理RHS的expression
    return ParseBinOpRHS(0, std::move(LHS));
}

/*
    parse RHS expression, 处理binary operator右边的expression

    ExprPrec: expression优先级的阈值, 如果RHS bianry expression优先级低于此阈值, 不进行处理 (就是operator优先级)
    LHS: 本RHS expression的LHS expression

    ParseBinOpRHS不会改变expression中元素的顺序, 而是会把优先级高的BinOpExpr先组合成ast。eg.  a+b*c = a + AST(b*c) = AST(a+AST(b*c))
*/
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
{
    while (true)
    {
        // 获取当前token(operator)优先级
        int TokPrec = GetTokenPrecedence();
        // 如果低于阈值, 不处理
        if (TokPrec < ExprPrec)
            return LHS;

        // 记录当前bianry operator
        int BinOp = CurTok;
        getNextToken(); // eay current binary operator

        // parse RHS expression
        auto RHS = ParseBasicExpression();
        if (!RHS)
            return nullptr;

        // 记录RHS expression后面的binary operator
        int NextTokPrec = GetTokenPrecedence();

        // 如果后面的binary operator优先级更高, 则先parse 后面的包括RHS的expression
        if (TokPrec < NextTokPrec)
        {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
            {
                return nullptr;
            }
        }

        // 合并LHS和RHS为一个Binary Expression AST
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

// basic expression的parser分配器
static std::unique_ptr<ExprAST> ParseBasicExpression()
{
    switch (CurTok)
    {
    default:
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

// parse the numeric expression
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto v = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(v);
}

// parse the parentheses with expression
static std::unique_ptr<ExprAST> ParseParenExpr()
{
    // eat (
    getNextToken();

    // parse the expression inside the parentheses
    auto v = ParseExpression();

    // detect the error
    if (!v)
        return nullptr;
    if (CurTok != ')')
        return LogError("expected ')'");

    // eat )
    getNextToken();

    return v;
}

// parse the identifier
static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string IdName = IdentifierStr;
    getNextToken(); // eat Identifier

    // 当作Variable处理
    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    // 当作function处理
    getNextToken(); // eat (
    // 读取args
    std::vector<std::unique_ptr<ExprAST> > args;
    if (CurTok != ')')
    {
        while (true)
        {
            // 储存arg
            if (auto arg = ParseExpression())
            {
                args.push_back(arg); // Args.push_back(std::move(Arg));
            }
            else
            {
                return nullptr;
            }

            // 如果是')'
            if (CurTok == ')')
            {
                break;
            }

            // 如果arg之间不是','
            if (CurTok != ',')
            {
                return LogError("Expected ',' or ')' in arguments")
            }
            // 吃掉','
            getNextToken();
        }
    }
    // 吃掉)
    getNextToken() return std::make_unique<CallExprAST>(IdName, std::move(args));
}

/**************************** Binary Parse ****************************/
// 储存binary operator
std::map<char, int> BinopPrecedence;

// 检测和返回CurrToken是否是binary operator, 如果不是就返回 -1
static int GetTokenPrecedence()
{
    // 如果CurrToken不是isascii码, 则肯定不是operator
    if (!isascii(CurTok))
        return -1;

    // 如果CurrToken不在binop表中, 咋不是operator
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}
/**************************** Parse the prototype ****************************/
// parse 'def'
static std::unique_ptr<FunctionAST> ParseDefinition()
{
    getNextToken(); // eat def.
    // 读取function 原型
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;
    // 读取function body
    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

// parse 'extern'
static std::unique_ptr<PrototypeAST> ParseExtern()
{
    getNextToken(); // eat extern.
    return ParsePrototype();
}

// parse 'def' 和 'extern' 后面的函数原型
static std::unique_ptr<PrototypeAST> ParsePrototype()
{
    // 判断def or extern后面跟的是不是函数原型
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    // 读取函数名
    std::string FnName = IdentifierStr;
    getNextToken();

    // 读取函数参数
    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");
    // 读取参数
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
    {
        ArgNames.push_back(IdentifierStr); // IdentifierStr在lexer中, 储存了string
    }
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");
    getNextToken();

    return std::make_unique<PrototypeAST>(FnName, ArgNames);
}

/*************************** Top-level parser ****************************/
static std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (auto E = ParseExpression())
    {
        // Make an anonymous proto.
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static void HandleDefinition()
{
    if (ParseDefinition())
    {
        fprintf(stderr, "Parsed a function definition.\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern()
{
    if (ParseExtern())
    {
        fprintf(stderr, "Parsed an extern\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr())
    {
        fprintf(stderr, "Parsed a top-level expr\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}