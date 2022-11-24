#include "locale.hxx"
#include "video.hxx"
#include "multiboot2.h"
#include "tty.hxx"
#include "vendor.hxx"
#include "font.hxx"

static constinit IconSet asciiIcons =
{
    .format = IconSet::Format::BLACK_WHITE,
    .width = 8,
    .height = 8,
    .nIcons = ARRAY_SIZE(asciiFont8x8) / 8,
    .buffer = (const uint8_t *)nullptr,
    .colorMap = nullptr,
};

static constinit IconSet tagalogIcons =
{
    .format = IconSet::Format::BLACK_WHITE,
    .width = 16,
    .height = 8,
    .nIcons = ARRAY_SIZE(tagalogFont16x8) / 8,
    .buffer = (const uint16_t *)nullptr,
    .colorMap = nullptr,
};

static constinit IconSet devanagariIcons =
{
    .format = IconSet::Format::BLACK_WHITE,
    .width = 16,
    .height = 8,
    .nIcons = ARRAY_SIZE(devanagariFont16x8) / 8,
    .buffer = (const uint16_t *)nullptr,
    .colorMap = nullptr,
};

static constinit IconSet systemIcons =
{
    .format = IconSet::Format::BLACK_WHITE,
    .width = 8,
    .height = 8,
    .nIcons = ARRAY_SIZE(systemFont8x8) / 8,
    .buffer = (const uint8_t *)nullptr,
    .colorMap = nullptr,
};

static constinit IconSet emojiIcons =
{
    .format = IconSet::Format::PLANE2,
    .width = 8,
    .height = 8,
    .nIcons = ARRAY_SIZE(emojiBitmap8x8) / 8,
    .buffer = (const uint16_t *)nullptr,
    .colorMap = nullptr,
};

Framebuffer g_KFrameBuffer;

Framebuffer::Framebuffer(multiboot_tag_framebuffer &tagfb)
{
    asciiIcons.buffer = (const uint8_t *)&asciiFont8x8;
    tagalogIcons.buffer = (const uint16_t *)&tagalogFont16x8;
    devanagariIcons.buffer = (const uint16_t *)&devanagariFont16x8;
    systemIcons.buffer = (const uint8_t *)&systemFont8x8;
    emojiIcons.buffer = (const uint16_t *)&emojiBitmap8x8;
    static Color emojiColorMap4[4] =
    {
        Color(0x000000),
        Color(0xFF00FF),
        Color(0xFFFF00),
        Color(0xFFFFFF)
    };
    emojiIcons.colorMap = (const Color *)&emojiColorMap4;

    switch (tagfb.common.type)
    {
    case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
        this->mode = Framebuffer::Mode::INDEXED;
        break;
    case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
        this->mode = Framebuffer::Mode::RGB;
        break;
    default: // Fallback to assuming RGB
        this->mode = Framebuffer::Mode::RGB;
        break;
    }

    this->addr = (decltype(this->addr))tagfb.common.framebuffer_addr;
    this->width = tagfb.common.framebuffer_width;
    this->height = tagfb.common.framebuffer_height;
    this->bpp = tagfb.common.framebuffer_bpp;
    this->pitch = tagfb.common.framebuffer_pitch;
    TTY::Print("fb: %ux%u (pitch=%u,bpp=%u) at %p\n", this->width, this->height,
               this->pitch, this->bpp, this->addr);
}

void Framebuffer::PlotPixel(unsigned x, unsigned y, Color color)
{
    if (x >= this->width || y >= this->height)
        return;

    switch (this->mode)
    {
    case Framebuffer::Mode::RGB:
        switch (this->bpp)
        {
        case 32:
            this->rgb32_addr[x + y * (this->pitch / sizeof(uint32_t))] = color.rgba;
            break;
        case 24:
        {
            uint8_t r = (color.rgba >> 16) & 0xFF;
            uint8_t g = (color.rgba >> 8) & 0xFF;
            uint8_t b = color.rgba & 0xFF;
            this->rgb8_addr[0 + x * 3 + y * this->pitch] = b;
            this->rgb8_addr[1 + x * 3 + y * this->pitch] = g;
            this->rgb8_addr[2 + x * 3 + y * this->pitch] = r;
        }
        break;
        case 16:
            this->rgb16_addr[x + y * (this->pitch / sizeof(uint16_t))] = color.rgba;
            break;
        default:
            break;
        }
        break;
    case Framebuffer::Mode::INDEXED:
        switch (this->bpp)
        {
        case 32:
            this->indexed_addr[x * sizeof(uint32_t) + y * this->pitch] = color.index;
            break;
        case 8:
            this->indexed_addr[x + y * this->pitch] = color.index;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

Color Framebuffer::GetPixel(unsigned x, unsigned y)
{
    Color col;
    if (x >= this->width || y >= this->height)
        return Color(0x000000);

    switch (this->mode)
    {
    case Framebuffer::Mode::RGB:
        switch (this->bpp)
        {
        case 32:
            return Color(this->rgb32_addr[x + y * (this->pitch / sizeof(uint32_t))]);
        case 24:
            col.r = this->rgb8_addr[0 + x * 3 + y * this->pitch];
            col.g = this->rgb8_addr[1 + x * 3 + y * this->pitch];
            col.b = this->rgb8_addr[2 + x * 3 + y * this->pitch];
            return col;
        case 16:
            return Color(this->rgb16_addr[x + y * (this->pitch / sizeof(uint16_t))]);
        default:
            break;
        }
        break;
    case Framebuffer::Mode::INDEXED:
        switch (this->bpp)
        {
        case 32:
            col.index = this->indexed_addr[x * sizeof(uint32_t) + y * this->pitch];
            return col;
        case 8:
            col.index = this->indexed_addr[x + y * this->pitch];
            return col;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return Color(0x000000);
}

void Framebuffer::FillSquare(unsigned x, unsigned y, unsigned w, unsigned h,
                             Color color)
{
    for (unsigned i = x; i < x + w; i++)
        for (unsigned j = y; j < y + h; j++)
            this->PlotPixel(i, j, color);
}

#define ABS(x) (((x) < 0) ? (-(x)) : (x))

void Framebuffer::PlotLine(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
                           Color color)
{
    int dx = x2 - x1; // Now do actual linely stuff
    int dy = y2 - y1;
    int sdx = ((dx < 0) ? -1 : ((dx > 0) ? 1 : 0));
    int sdy = ((dy < 0) ? -1 : ((dy > 0) ? 1 : 0));
    int dxabs = ABS(dx);
    int dyabs = ABS(dy);
    int x = (dyabs >> 1);
    int y = (dxabs >> 1);
    int px = x1;
    int py = y1;
    this->PlotPixel(px, py, color);
    if (dxabs >= dyabs)
    {
        for (int i = 0; i < dxabs; i++)
        {
            y += dyabs;
            if (y >= dxabs)
            {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;
            this->PlotPixel(px, py, color);
        }
    }
    else
    {
        for (int i = 0; i < dyabs; i++)
        {
            x += dxabs;
            if (x >= dyabs)
            {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;
            this->PlotPixel(px, py, color);
        }
    }
}

void Framebuffer::PlotChar(unsigned x, unsigned y, char32_t ch, Color color)
{
    advance_x = 0;
    advance_y = 0;
    if (ch < 0x20)
        return;

    if (ch <= 0xFF) // American ASCII
    {
        // Font mappings are coded in ASCII
        // 0x20 = ' ' in ascii, the first character of our font bitmap is a space
        ch = Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>(ch);
        ch -= 0x20;
        const auto &asciiGlyph = asciiIcons.GetIcon<uint8_t>(ch);
        for (size_t cy = 0; cy < 8; cy++)
        {
            for (size_t cx = 0; cx < 8; cx++)
            {
                if (!(asciiGlyph[cy] & (1 << cx)))
                    continue;
                this->PlotPixel(x + 7 - cx, y + cy, color);
            }
        }
        advance_x = 8;
    }
    else if (ch >= 0x900 && ch <= 0x970) // Devanagari block
    {
        // bool composite = ch >= 0x1712 && ch <= 0x1715;
        bool composite = false;
        ch -= 0x900;
        const auto &devanagariGlyph = devanagariIcons.GetIcon<uint16_t>(ch);
        for (size_t cy = 0; cy < 8; cy++)
        {
            for (size_t cx = 0; cx < 16; cx++)
            {
                if (!(devanagariGlyph[cy] & (1 << cx)))
                    continue;
                this->PlotPixel(x + 15 - cx, y + cy, color);
            }
        }
        advance_x = composite ? 0 : 16;
    }
    else if (ch >= 0x1700 && ch <= 0x171F) // Tagalog block
    {
        bool composite = ch >= 0x1712 && ch <= 0x1715;
        ch -= 0x1700;
        const auto &tagalogGlyph = tagalogIcons.GetIcon<uint16_t>(ch);
        for (size_t cy = 0; cy < 8; cy++)
        {
            for (size_t cx = 0; cx < 16; cx++)
            {
                if (!(tagalogGlyph[cy] & (1 << cx)))
                    continue;
                this->PlotPixel(x + 15 - cx, y + cy, color);
            }
        }
        advance_x = composite ? 0 : 16;
    }
    else if (ch >= 0x1F600 && ch <= 0x1F64F) // Emoticon block
    {
        ch -= 0x1F600;
        const auto &emoticonGlyph = emojiIcons.GetIcon<uint16_t>(ch);
        for (size_t cy = 0; cy < 8; cy++)
        {
            for (size_t cx = 0; cx < 8; cx++)
            {
                uint8_t index = (emoticonGlyph[cy] >> (cx * 2)) & 0x03;
                this->PlotPixel(x + 7 - cx, y + cy, const_cast<Color&>(emojiIcons.colorMap[index]));
            }
        }
        advance_x = 8;
    }
}

void Framebuffer::PlotPolygon(unsigned x, unsigned y, const Vector3D nodes[], unsigned nNodes, Color color)
{
    if (nNodes < 1)
    {
        if (nNodes) // 1 node is a single pixel
            this->PlotPixel(x + nodes[0].x, y + nodes[0].y, color);
        return;
    }

    for (unsigned int i = 0; i < nNodes; i++)
    {
        const auto& a = nodes[i];
        const auto& b = i < nNodes - 1 ? nodes[i + 1] : nodes[0];
        this->PlotLine(x + a.x, y + a.y, x + b.x, y + b.y, color);
    }
}

const uint8_t *Framebuffer::GetMouseIcon()
{
    switch (this->mouseMode)
    {
    case Framebuffer::MouseMode::NORMAL:
        return systemIcons.GetIcon<uint8_t>(0);
    case Framebuffer::MouseMode::WAIT:
        if (this->mouseKeyframe >= 4)
            this->mouseKeyframe = 0;
        return systemIcons.GetIcon<uint8_t>(1 + (this->mouseKeyframe++));
    }
    return systemIcons.GetIcon<uint8_t>(0);
}

void Framebuffer::DrawMouse()
{
    const auto &mouseIcon = this->GetMouseIcon();
    for (unsigned int i = 0; i < 8; i++)
    {
        for (unsigned int j = 0; j < 8; j++)
        {
            if (!(mouseIcon[j] & (1 << i)))
                continue;
            Color col = this->GetPixel(this->mouse_x + 7 - i, this->mouse_y + j);
            if (col.r > 0x80 || col.g >= 0x80 || col.b >= 0x80)
                this->PlotPixel(this->mouse_x + 7 - i, this->mouse_y + j, Color(0x000000));
            else
                this->PlotPixel(this->mouse_x + 7 - i, this->mouse_y + j, Color(0xFFFFFF));
        }
    }
}

void Framebuffer::MoveMouse(unsigned new_x, unsigned new_y)
{
    if (this->mouse_x == new_x && this->mouse_y == new_y)
    {
        for (unsigned int i = 0; i < 8; i++)
            for (unsigned int j = 0; j < 8; j++)
                this->PlotPixel(this->mouse_x + i, this->mouse_y + j, this->mouse_bf[i + j * 8]);
        this->DrawMouse();
        return;
    }

    // Restore the old buffer at the mouse
    for (unsigned int i = 0; i < 8; i++)
        for (unsigned int j = 0; j < 8; j++)
            this->PlotPixel(this->mouse_x + i, this->mouse_y + j, this->mouse_bf[i + j * 8]);

    this->mouse_x = new_x;
    this->mouse_y = new_y;
    for (unsigned int i = 0; i < 8; i++)
        for (unsigned int j = 0; j < 8; j++)
            this->mouse_bf[i + j * 8] = this->GetPixel(this->mouse_x + i, this->mouse_y + j);

    this->DrawMouse();
}
