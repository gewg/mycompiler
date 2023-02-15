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
// 普通expression的parser的入口
static std::unique_ptr<ExprAST> ParsePrimary()
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

            // 如果是)
            if (CurTok == ')')
            {
                break;
            }

            // 如果arg之间不是,
            if (CurTok != ',')
            {
                return LogError("Expected ',' or ')' in arguments")
            }
            // 吃掉,
            getNextToken();
        }
    }
    // 吃掉)
    getNextToken()
    return std::make_unique<CallExprAST>(IdName, std::move(args));
}

/**************************** Binary Parse ****************************/
