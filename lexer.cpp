/**
 * Auther: Wei Ge
 * Date: 23/1/18
 * Lexer: Break the inputed language into tokens
 */

#include <stdio.h>
#include <string>
#include "lexer.h"

std::string IdentifierStr;
double NumVal;

/**
 * Transfer the input to token, each time outputs one token
 */
int gettok()
{
    static int LastChar = ' ';

    // skip the whitespace
    while (isspace(LastChar))
    {
        LastChar = getchar();
    }

    // recognize the input as identifier
    if (isalpha(LastChar))
    {

        // get the whole phrase
        while (isalpha(LastChar))
        {
            IdentifierStr += LastChar;
            LastChar = getchar();
        }

        // identify the identifier
        if (IdentifierStr == "def")
        {
            return tok_def;
        }
        if (IdentifierStr == "extern")
        {
            return tok_extern;
        }

        // if the identifier is unknown, output the tok_identifier
        return tok_identifier;
    }

    // recognize the input as number
    if (isdigit(LastChar) || LastChar == '.')
    {
        std::string NumStr;

        // get the whole phrase
        while (isdigit(LastChar) || LastChar == '.')
        {
            NumStr += LastChar;
            LastChar = getchar();
        }

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // recognize the input as comment
    if (LastChar == '#')
    {
        // skip the whole comment
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r')
        {
            LastChar = getchar();
        }

        // 递归, 输出下一个token
        if (LastChar != EOF)
        {
            return gettok();
        }
    }

    // recognize the input as end of file
    if (LastChar == EOF)
    {
        return tok_eof;
    }

    // otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}