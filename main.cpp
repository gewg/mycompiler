#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(){
    // 根据等级排列operator
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest

    MainParser();
}