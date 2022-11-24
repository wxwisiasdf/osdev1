/// hello.cxx
/// JDA+ a reincarnation of the original mainframe JDA for UDOS

#include <cstddef>
#include <kernel/tty.hxx>
#include <kernel/video.hxx>

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

__attribute__((section(".data.startup"))) int UDOS_32Main(char32_t[])
{
    TTY::Print("Hello world\n");

    for (size_t i = 0; i < 32; i++)
    {
        g_KFrameBuffer.PlotChar(i * 8, 0, (uint8_t)('A' + i), Color(0xFFFFFFFF));
        g_KFrameBuffer.PlotChar(i * 16, 12, 0x1700 + i, Color(0xFFFFFFFF));
        g_KFrameBuffer.PlotChar(i * 16, 20, 0x900 + i, Color(0xFFFFFFFF));
        g_KFrameBuffer.PlotChar(i * 16, 30, 0x920 + i, Color(0xFFFFFFFF));
    }

    {
        const auto *tagname = U"ᜏᜒᜃᜅ᜔ ᜆᜄᜎᜓᜄ᜔";
        int tx = 0;
        for (size_t i = 0; i < 13; i++)
        {
            g_KFrameBuffer.PlotChar(128 + tx, 96, tagname[i], Color(0xFF00FF00));
            tx += g_KFrameBuffer.advance_x;
        }
    }

    {
        const auto *tagname = U"नागरी देवनागरी नगरम्";
        int tx = 0;
        for (size_t i = 0; i < 21; i++)
        {
            g_KFrameBuffer.PlotChar(128 + tx, 96 + 16, tagname[i], Color(0xFF00FF00));
            tx += g_KFrameBuffer.advance_x;
        }
    }

    {
        const auto *tagname = "This is in english";
        int tx = 0;
        for (size_t i = 0; i < 19; i++)
        {
            g_KFrameBuffer.PlotChar(128 + tx, 96 + 24, (uint8_t)(tagname[i]), Color(0xFF00FF00));
            tx += g_KFrameBuffer.advance_x;
        }
    }

    {
        const auto *tagname = "Y ESTE TEXTO NO SIRVE";
        int tx = 0;
        for (size_t i = 0; i < 33; i++)
        {
            g_KFrameBuffer.PlotChar(128 + tx, 96 + 32, (uint8_t)(tagname[i]), Color(0xFF00FF00));
            tx += g_KFrameBuffer.advance_x;
        }
    }

    {
        const auto *tagname = U"\x01F60D";
        int tx = 0;
        for (size_t i = 0; i < 18; i++)
        {
            g_KFrameBuffer.PlotChar(tx, 96 + 32, tagname[i], Color(0xFF00FF00));
            tx += g_KFrameBuffer.advance_x + 2;
        }
    }

    while (1)
        ;
    return 0;
}
