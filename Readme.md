## Summary

* lexer
  * 读取char, 组合成词, 每个词为一个token
* parser
  * 读取token, 组合成ast
* whole process
  1. lexer: getToken()
  2. 
  3. Parser: switch(CurToken), 决定使用哪个Parser
  4. 

## Lexer

* 词法分析: divide program text into tokens

* 实现步骤

  1. 声明token类型

  2. 判断输入类型, 并输出对应的token类型:

     * 判断是否是identifier

     * 判断是否是数字

     * 判断是否是注释

     * 判读是否是文件结尾

## Parser

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

## Expression Parser

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

* **parse (binary)**
