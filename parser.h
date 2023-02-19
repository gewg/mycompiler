
// ast
class ExprAST
{
public:
    virtual ~ExprAST();
};

// the expression for numeric literals
class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val);
    Value *codegen();
};

// the expression for referencing a variable
class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(const std::string &Name);
};

// the expression for binary operator
class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS);
};

// the expression for function calls
class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST> > Args;

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST> > Args);
};

class PrototypeAST
{
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> Args);

    const std::string &getName() const;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body);
};

void MainParser();
int getNextToken();
std::unique_ptr<ExprAST> LogError(const char *Str);

// basic expression parser
static std::unique_ptr<ExprAST> ParseBasicExpression();
static std::unique_ptr<ExprAST> ParseNumberExpr();
static std::unique_ptr<ExprAST> ParseParenExpr();
static std::unique_ptr<ExprAST> ParseIdentifierExpr();

// binary expression parser
extern std::map<char, int> BinopPrecedence;
static int GetTokenPrecedence();
static std::unique_ptr<ExprAST> ParseAllExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

// prototype parser
static std::unique_ptr<FunctionAST> ParseDefinition();
static std::unique_ptr<PrototypeAST> ParseExtern();
static std::unique_ptr<PrototypeAST> ParsePrototype();

// top-level
static std::unique_ptr<FunctionAST> ParseTopLevelExpr();
static void HandleDefinition();
static void HandleExtern();
static void HandleTopLevelExpression();