/**
 * Auther: Wei Ge
 * Date: 23/1/19
 * Parser: Input token, output the AST
 */

#include <stdio.h>
#include <string>
#include <memory>
#include <vector>

#include "lexer.h"

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
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
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
static int getNextToken()
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

/**************************** Basic Expression parse ****************************/

// parse expressions
// 所有Expression = basic expression + binary operator
static std::unique_ptr<ExprAST> ParseAllExpression()
{
    // 首先parse LHS basic expression
    auto LHS = ParseBasicExpression();
    if (!LHS) return nullptr;

    // 然后处理RHS的expression
    return ParseBinOpRHS(0, std::move(LHS));
}

/*
    parse RHS expression, 处理binary operator右边的expression

    ExprPrec: expression优先级的阈值, 如果RHS bianry expression优先级低于此阈值, 不进行处理 (就是operator优先级)
    LHS: 本RHS expression的LHS expression
*/
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) 
{
    while (true)
    {
        // 获取当前token(operator)优先级
        int TokPrec = GetTokenPrecedence();
        // 如果低于阈值, 不处理
        if (TokPrec < ExprPrec) return LHS;

        // 记录当前bianry operator
        int BinOp = CurTok;
        getNextToken(); // eay current binary operator

        // parse RHS expression
        auto RHS = ParseBasicExpression();
        if (!RHS) return nullptr;

        // 记录RHS expression后面的binary operator
        int NextTokPrec = GetTokenPrecedence();

        // 如果后面的binary operator优先级更高, 则先parse 后面的包括RHS的expression
        if (TokPrec < NextTokPrec)
        {
            RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
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
    getNextToken()
    return std::make_unique<CallExprAST>(IdName, std::move(args));
}

/**************************** Binary Parse ****************************/
// 储存binary operator
static std::map<char, int> BinopPrecedence;

// 检测和返回CurrToken是否是binary operator, 如果不是就返回 -1
static int GetTokenPrecedence()
{
    // 如果CurrToken不是isascii码, 则肯定不是operator
    if (!isascii(CurTok)) return -1;

    // 如果CurrToken不在binop表中, 咋不是operator
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

int main()
{
    // 根据等级排列operator
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest
}