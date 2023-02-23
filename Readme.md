## Summary

* lexer
  * 读取char, 组合成词, 每个词为一个token
* parser
  * 读取token, 组合成ast
* **whole pipeline**
  * **code -> ast -> 调用ast的codegen生成value -> ir**
  
* **whole process**
  1. **Driver**调用**Lexer**, 获得一个token
  2. 根据token的类型, 调用 (**Parser**&**Codegen**)
     1. **Parser**根据token类型判断要生成的AST。然后持续调用**Lexer**和**别的Parser**, 直到生成完整AST
     2. 调用生成好的AST的**Codegen**, 生成和储存IR

# <u>Lexer</u>

* 词法分析: divide program text into tokens

* 实现步骤

  1. 声明token类型

  2. 判断输入类型, 并输出对应的token类型:

     * 判断是否是identifier

     * 判断是否是数字

     * 判断是否是注释

     * 判读是否是文件结尾



# <u>Parser</u>

* 语法分析: read token, store the information from token to AST

* 实现步骤

  1. 为大部分不同的Token类型, 声明对应的AST类型

     one-to-one

     * 忽略**括号**token

  2. 为不同的AST类型, 生成转换function的parser。Parser接收的类型分为Basic expression 和 Binary expression生成

     * numeric
     * parentheses
     * identifier

  3. driver

     * 各种parser的入口
  
* Tips

  * 在每一个Parser中会**<u>更新CurToken的值到下一位</u>**, 这是为了在处理expression时, 可以递归处理其中的每一个元素

  * **parse binary expression**

    * 使用Operator-Precedence Parsing, 基于操作符优先级。

    * bianry expression = basic expression + binary operator.

      所以parse binary expression, 采取: 

      parse basic expression -> 看下一个token是否是operator -> parse basic expression -> 看下一个token是否是operator -> ...的流程

  * Expression

    * Basic: 简单表达式

      x = 10

    * Binary: 两个基本表示式组成

      y = x + 5

## Parser Driver

* **eof**
* **;**
  * 忽略
* **prototype** (函数原型)
  * **def**: function ast
  * **extern**: prototype ast
  * prototype ast为def和extern之后的函数的ast
* <u>basic expression</u> (default)
  * **number tok:** number ast
  * **parentheses tok**: parentheses ast
  * **identifier tok**
    * 使用variable ast
    * 储存为call ast
* <u>binary expression</u> (default)
  * **bianry expression** ast

## Top Level Parser

* 封装parser, 添加了打印信息

## AST Parser

* **parse number (basic)**

  * 调用: CurTok == tok_number
  * step
    1. 读取**NumVal**，存入NumberExprAST
    2. 把CurTok移动到下一位
    3. 返回NumberExprAST

* **parse parentheses expression (basic)**

  * 调用: CurTok == '('

  * step

    1. 调用getNextToken, 吃掉(

    2. 调用ParseExpression, 处理expression

       V = ParseExpression()

    3. 如果V为空, return nullptr

    4. 如果没有')', 报错

    5. 调用getNextToken, 吃掉)

  * 括号的作用

    * While we could do it this way, the most important role of parentheses are to guide the parser and provide grouping. Once the parser constructs the AST, parentheses are not needed.

* **parse identifier (basic)**

  * 调用: CurTok == tok_identifier
  * step
    1. 储存identifier为IdName
    2. 调用getNextToken, 吃掉identifier
    3. 如果CurTok不是'(', 把CurTok当作Variable处理
    4. 如果CurTok是'(', 当作function处理。调用getNextToken, 吃掉(
    5. 创造 std::vector<std::unique_ptr<ExprAST>> Args
    6. 如果CurTok不是'(', 遍历读取args
    7. 吃掉')'

## Binary Expression Parser

* **ParseExpression (处理所有的expression的入口)**

* **ParseBinOpRHS (处理binary operator)**

  * step

    1. parse **basic expression (LHS)**

    2. parse binary operator为**TokPrec**, 如果<u>优先级不够或者不是binary op</u>, 则直接return LHS

       * 使用GetTokPrecedence()

    3. parse **下一个basic expression (RHS)**

    4. parse 下一个binary operator为**NextPrec**

    5. 比较**TokPrec**和**NextPrec**

       * 如果TokPrec < NextPrec, 递归调用ParseBinOpRHS, 先parse后一个包括RHS的expression

         **<u>(只比较相邻的两个Operator就可以确定是否可以计算当前binary expression, 不需要在整个expression中找到优先级最高的binary expression先计算)</u>**

    6. 合并LHS和RHS

  * tips

    * 因为'( )'被当作basic expression会直接计算, 所以binary expression中不需要考虑括号的优先级

    * ParseBinOpRHS不会改变expression中元素的顺序, 而是会把优先级高的BinOpExpr先组合成ast, 从而变成内层的ast

      eg. a + b * c = a + AST(b * c) = AST(a + AST(b*c))

## Prototype Parser

* 处理<u>函数原型 (储存函数信息)</u>
  * def
    * 读取函数原型后, 会再读取function的body
  * extern
* step
  1. 判断是否是prototype
  2. 读取函数名
  3. 判断是否有'( )', 并读取和储存函数参数。参数储存为string
  4. 返回**函数名**和**函数参数vector**



# <u>codegen</u>

* **Transfer information from AST into IR**
  * 每个AST拥有codegen方法
  * codegen: 收集AST中的信息, 组合处理后, 传递信息到IRBuilder, 生成Value or Function
* **实现步骤**
  1. 给ExprAST(所有AST的父类)添加<u>codegen()</u>, 此function返回LLVM Value object or Function Object的内存 (不使用智能指针, 直接使用指针)
  2. 创建变量
     * **LLVMContext**
       * 黑盒子。拥有大量core llvm data, eg. constant value table
     * **IRBuilder**
       * 保存<u>插入新instruction的地方</u>
       * 拥有<u>创建新instruction的方法</u>
     * **Module**
       * LLVM IR用于储存代码的结构
         * 包括全部Value object的内存 (函数和全局变量)
       * 用例
         * 转换call expression ast时, 会根据name从Module中取出关于函数的信息
     * **NamedValues (map<string, value>)**
       * 保存了映射关系: **LLVM Value的名字**和**LLVM Value本身**
  3. 把不同类型的AST转换成Value
     1. Expression AST (Basic和Binary)
     2. Function AST (prototype)
* **属性llvm::Value和llvm::Function**
  * codegen()产出的static single assignment
  * 为IR中的代码段, Value组成IR

## Expression AST -> Value(IR中的代码段)

* **Number expression**

  * llvm中储存numberic: ConstantFP

* **Variable expression**

  * step
    1. 使用NamedValues表获得variable的LLVM Value
    2. 返回llvm value

* **Binary expression**

  * step

    1. 获得binary operator两边expression的llvm value, 通过调用**LHS**和**RHS**的codegen
    2. 根据binary operator的种类, 调用IRBuilder的插入新instruction的方法, 添加binary expression到ir

  * 转换时, 会递归转换LHS和RHS。所以会**先转换最内层的LHS和RHS**。而**最内层的LHS和RHS是优先级高的Binary Operate Expression**

    AST结构例子: AST(a+AST(b*c))。会先转换AST(b * c)成Value, 添加到IR

* **Call expression**

  * step
    1. 根据callee name， 从**TheModule**中获得对应的function **CalleeF**
    2. 核对全局变量args的数量 和 **CalleeF**中记录的args数量
    3. 遍历全局变量args, 挨个codegen, 储存到vector<Value*>。每次储存后检查codegen后结果
    4. 调用IRBuilder的插入新instruction的方法, 添加call expression到ir

## Fucntion AST -> Function(IR中的代码段)

* **Prototype**

  * step
    1. 创建Vector<Type*> 储存args type
    2. 创建**FunctionType**, 包括: 参数类型, 返回值类型, etc.
    3. 创建**Function**, 包括: function type, linkage(因为function可能是在外部定义), name
    4. 给arguements设置名字, 让IR可读性更高

* **Function**

  * 本compiler中, function的body只会有一句binary expression

  * step

    1. 检查**function prototype**是否被codegen过

       * 如果没有，则先codegen prototype

    2. 检查**function**是否是empty的

       * 如果不是empty的，则说明应该redefined了，要报error

    3. 创建**Basic Block**, 用于储存function body。并使用**IRBuilder**设置接下来的instruction要被插入这个Block中

    4. 把arguments填充进NamedValue

    5. codegen **function body**。并使用**IRBuilder**生成ret value

       -------- 到此, function ast codegen 完成 ------

    6. 使用**verifyFunction()**检测codegen的结果有没有问题。如果没有问题，return **FunctionObject**

    7. 如果有问题, 清空这个function



# <u>Optimization</u>
