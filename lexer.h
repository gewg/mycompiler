#include <string>

// type of token
enum Token
{
    // end of file
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();