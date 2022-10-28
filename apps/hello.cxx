/// jdaplus.cxx
/// JDA+ a reincarnation of the original mainframe JDA for UDOS

#include <cstddef>
#include <kernel/tty.hxx>
#include <kernel/video.hxx>

int UDOS_32Main(char32_t[])
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
