/**
 * Auther: Wei Ge
 * Date: Feb 20, 2023
 * Parser: Get information from AST, output the IR
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
// number expression
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
// binary expression
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
// call expression
Value *CallExprAST::codegen()
{
    // get the information of the function
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // 检查argument数量
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

// 转换args
std:
    vector<Value *> ArgsV;
    for (unsigned i = 0; e = Args.size(); i != e; ++i)
    {
        ArgsV.push_back(Args[i]->codegen());
        // 每次push进一个variable, 检查这个variable是否正常
        if (!ArgsV.back())
            return nullptr;
    }

    // 转换callee
    return Builder->CreateCall(CalleF, ArgsV, "calltmp");
}

/*************************** 转换function到IR ***************************/
Function *PrototypeAST::codegen()
{
    // all function arguments in Kaleidoscope are of type double
    // 创建vector储存args
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    // 创建FunctionType保存function信息
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
    // 创建function
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // 给arguements设置名字, 可以让IR可读性更高
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function *FunctionAST::codegen()
{
    // 检查function是否通过prototype被codegen过
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    // 如果没有, 进行codegen
    if (!TheFunction)
        TheFunction = Proto->codegen();
    if (!TheFunction)
        return nullptr;

    // 检查function是否有body, 如果有则说明一个函数被定义了两次
    if (!TheFunction->empty())
        return (Function *)LogErrorV("Function cannot be redefined.");

    // Create a new basic block (entry) to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB); // 接下来instruction被插入到entry中

    // Record the function arguments in the NamedValues map.
    NamedValues.clear(); // fixme - why clean it
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    // codegen function body
    if (Value *RetVal = Body->codegen())
    {
        // Finish off the function.
        Builder->CreateRet(RetVal);
        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);
        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}