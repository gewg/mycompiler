/**
 * Auther: Wei Ge
 * Date: Feb 20, 2023
 * Parser: Input AST, output the IR
 */

#include "lexer.h"
#include "parser.h"

// 保存llvm环境
static std::unique_ptr<LLVMContext> TheContext;
// 记录:如何插入新instruction
static std::unique_ptr<IRBuilder<>> Builder(TheContext);
// 保存: 全部Value
static std::unique_ptr<Module> TheModule;
// 映射表: ast中保存的值 和 IR
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str)
{
    LogError(Str);
    return nullptr;
}

/*************************** 转换expression到IR ***************************/
// numberic
Value *NumberExprAST::codegen()
{
    return ConstantFP::get(*TheContext, APFloat(Val));
}
// variable
Value *VariableExprAST::codegen()
{
    // 这里的Name为VariableExprAST的成员变量
    Value *V = NamedValues[Name];
    if (!V)
        LogErrorV("Unknown variable name");
    return V;
}
Value *BinaryExprAST::codegen()
{
    // 获得bianry operator两边expression的IR
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    // 根据binary operator, 添加IR
    switch (Op)
    {
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return Builder->CreateFMul(L, R, "multmp");
    case '<':
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return Builder->CreateUIToFP(L, Type::getDoubleTy(TheContext),
                                     "booltmp");
    default:
        return LogErrorV("invalid binary operator");
    }
}