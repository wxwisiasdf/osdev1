#ifndef FB_HXX
#define FB_HXX 1

#include <cstdint>
#include <variant>

struct multiboot_tag_framebuffer;
union Color
{
    union
    {
        uint8_t index;
        uint32_t bgr;
        uint32_t bgra;
        uint32_t rgb;
        uint32_t rgba;
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
    };

    Color() = default;
    Color(uint32_t _rgba)
        : rgba{_rgba}
    {
    }
    Color(Color &) = default;
    Color(Color &&) = default;
    Color &operator=(const Color &) = default;
    ~Color() = default;
};

struct Vector3D
{
    unsigned int x, y, z;
};

class Framebuffer
{
    enum Mode
    {
        INDEXED,
        RGB,
    } mode;
    union
    {
        void *addr;
        uint8_t *indexed_addr;
        uint32_t *rgb32_addr;
        uint16_t *rgb16_addr;
        uint8_t *rgb8_addr;
    };
    Color mouse_bf[8 * 8];
    unsigned mouse_x = 0, mouse_y = 0;
    unsigned pitch;
    unsigned char bpp;

public:
    Framebuffer() = default;
    Framebuffer(multiboot_tag_framebuffer &tagfb);
    Framebuffer(Framebuffer &) = delete;
    Framebuffer(Framebuffer &&) = delete;
    Framebuffer &operator=(const Framebuffer &) = default;
    ~Framebuffer() = default;

    void PlotPixel(unsigned x, unsigned y, Color color);
    Color GetPixel(unsigned x, unsigned y);
    void FillSquare(unsigned x, unsigned y, unsigned w, unsigned h,
                    Color color);
    void PlotLine(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
                  Color color);
    void PlotChar(unsigned x, unsigned y, char32_t ch, Color color);
    void PlotPolygon(unsigned x, unsigned y, const Vector3D nodes[], unsigned nNodes, Color color);
    const uint8_t *GetMouseIcon();
    void DrawMouse();
    void MoveMouse(unsigned new_x, unsigned new_y);

    unsigned width;
    unsigned height;

    // Text rendering
    int advance_x;
    int advance_y;

    enum MouseMode
    {
        NORMAL,
        WAIT,
    } mouseMode = MouseMode::NORMAL;
    int mouseKeyframe;
};

extern Framebuffer g_KFrameBuffer;

struct IconSet
{
    enum Format
    {
        BLACK_WHITE,
        PLANE2, // 2-bits each
        PLANE4, // 4-bits each
        RGBA,
    } format = Format::RGBA;
    unsigned short width;
    unsigned short height;
    unsigned char nIcons;
    std::variant<const uint8_t *, const uint16_t *, const uint32_t *> buffer;
    const Color *colorMap;

    template <typename T>
    const T *GetIcon(unsigned char i) const
    {
        if (i > this->nIcons)
            return (const T *)nullptr;

        switch (this->format)
        {
        case Format::BLACK_WHITE:
            return &(std::get<const T *>(buffer)[i * height]);
        case Format::PLANE2:
            return &(std::get<const T *>(buffer)[i * height]);
        case Format::RGBA:
            return &(std::get<const T *>(buffer)[i * height]);
        default:
            break;
        }
        return (const T *)nullptr;
    }
};

#endif
