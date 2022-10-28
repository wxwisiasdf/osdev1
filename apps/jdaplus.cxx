/// jdaplus.cxx
/// JDA+ a reincarnation of the original mainframe JDA for UDOS

#include <cstddef>
#include <kernel/tty.hxx>

namespace JDAPLUS
{
    struct Token
    {
        enum Type
        {
            PLUS,
            MINUS,
            DIVIDE,
            REMAINDER,
            MULTIPLY,
            BIT_AND,
            BIT_OR,
            LOGICAL_NOT,
            LOGICAL_AND,
            LOGICAL_OR,

            LPAREN,
            RPAREN,
            LBRACKET,
            RBRACKET,
            COMMA,
            DOT,
            
            // Keywords
            CLASS,
            MEMBERS,
            FUNCTION,
            CONSTRUCTOR,
            ASSIGN,
            TO,
            AS,
            HAS,
            INT,
            STRING,
            THIS,
            END = 0x80,
        };
    };

#define MAX_TOKENS 2048
    struct Lexer
    {
        Token tokens[MAX_TOKENS] = {};
        size_t n_tokens = 0;
    };
}

int main() {
    TTY::Print("Hello world");
    return 0;
}

#if 0
class MainWindow
    members
        has HWND win
        has int x
        has int y
        has int ox
        has int oy
    end-members

    function constructor (HWND win)
        Assign win to this.win
    end-function
end-class
#endif
