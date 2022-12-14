#ifndef UI_HXX
#define UI_HXX 1

#include <optional>
#include <atomic>
#include <variant>
#include <vector>
#include "video.hxx"
#include "tty.hxx"

namespace UI
{
struct Widget;
class Manager
{
    Framebuffer &fb;
    UI::Widget *drag_widget = nullptr;
    int drag_x = 0, drag_y = 0;

    // Box of area drawn over
    int drawn_x1 = 0, drawn_y1 = 0, drawn_x2 = 0, drawn_y2 = 0;
    size_t drawn_z_level = 0; // Z-level drawn (lower is higher)
    UI::Widget *rootWidget = nullptr;
public:
    Manager() = delete;
    Manager(Framebuffer &fb);
    Manager(Manager &) = delete;
    Manager(Manager &&) = delete;
    Manager &operator=(const Manager &) = delete;
    ~Manager() = default;
    static Manager &Get();
    void SetFramebuffer(Framebuffer &fb);
    void Draw(UI::Widget &w, int ox, int oy);
    void CheckRedraw(UI::Widget &w, int ox, int oy);
    void CheckUpdate(UI::Widget &w, unsigned mx, unsigned my, bool left, bool right, char32_t keypress);
    void Update();
    void SetRoot(UI::Widget& w);
    UI::Widget* GetRoot();
    void ErrorMsg(const char& title, const char& msg);
};

class Widget
{
protected:
    const char *tooltipText = nullptr;
    const char *text = nullptr;
    const char32_t *tooltipText32 = nullptr;
    const char32_t *text32 = nullptr;
    std::vector<UI::Widget *> children;
    /// @brief Whetever this widget needs to be redrawn
    bool needs_redraw = true;
    bool skeleton = false; // Only redraw skeleton
    bool dragable = false;
    bool clicked = false;
    bool inputted = false;

public:
    enum Flex
    {
        NONE,
        ROW,
        COLUMN,
    } flex = UI::Widget::Flex::NONE;
    Widget *parent = nullptr;
    unsigned short width = 0, height = 0;
    signed int x = 0, y = 0;
    signed int ox = 0, oy = 0;
    int padding_left = 2, padding_right = 2, padding_top = 2, padding_bottom = 2;
    std::atomic<bool> tooltipHovered;

    Widget();
    Widget(Widget &) = delete;
    Widget(Widget &&) = delete;
    Widget &operator=(const Widget &) = delete;
    virtual ~Widget();

    void Kill()
    {
        delete this;
    }
    void SetText(const char *text);
    void SetText(const char32_t *text);
    const char *GetText() const;
    void SetTooltipText(const char *text);
    void SetTooltipText(const char32_t *text);
    const char *GetTooltipText() const;
    void AddChildDirect(Widget &w);

    template<typename T, typename ...Ts>
    T& AddChild(Ts&&... args)
    {
        auto* p = new T(std::forward<Ts>(args...)...);
        this->AddChildDirect(*p);
        return *p;
    }

    void DrawShadow(Color outer_color, Color inner_color, Color mid_color);
    void Redraw();
    void SetSkeleton(bool value);
    void MoveBy(int off_x, int off_y);

    void DrawText(const char* text, int tx, int ty, int max_x, int max_y, int &final_x, int &final_y, bool &multiline, Color color);
    void DrawText(const char32_t* text, int tx, int ty, int max_x, int max_y, int &final_x, int &final_y, bool &multiline, Color color);
    virtual void DrawText(int tx, int ty, Color color);
    virtual void Draw();

    void (*OnHover)(UI::Widget &w, unsigned mx, unsigned my) = nullptr;
    void (*OnClick)(UI::Widget &w, unsigned mx, unsigned my,
                    bool left, bool right) = nullptr;
    void (*OnUpdate)(UI::Widget &w) = nullptr;
    void (*OnInput)(UI::Widget &w, char32_t keypress) = nullptr;
    friend class Manager;
};

struct Textbox : public UI::Widget
{
    char32_t textBuffer[128];
    size_t bufferPos;

    Textbox();
    virtual ~Textbox() = default;
    void Draw();
};

struct Button : public UI::Widget
{
    Button() = default;
    virtual ~Button() = default;
    void Draw();
};

struct Group : public UI::Widget
{
    Group();
    virtual ~Group() = default;
    void Draw();
};

struct Menu : public UI::Widget
{
    Menu();
    virtual ~Menu() = default;
    void Draw();
};

struct Taskbar : public UI::Widget
{
    Taskbar();
    virtual ~Taskbar() = default;
    void Draw();
};

struct Window : public UI::Widget
{
    Window();
    virtual ~Window() = default;
    void Draw();
    void Decorate();
    bool isClosed = false;
    std::optional<UI::Button> closeBtn;
    std::optional<UI::Button> minimBtn;
};

struct Viewport : public UI::Widget
{
    Viewport();
    virtual ~Viewport() = default;
    void Draw();
    void (*OnDraw)(UI::Viewport &w, unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey) = nullptr;
};

struct Terminal : public UI::Widget
{
    Terminal();
    virtual ~Terminal() = default;
    void Draw();
    TTY::BufferTerminal term;
};

struct DesktopIcon : public UI::Widget
{
    DesktopIcon();
    virtual ~DesktopIcon() = default;
    void Draw();
    size_t iconIndex = 0;
};

struct Desktop : public UI::Widget
{
    enum Background
    {
        SOLID,
        DIAGONAL_LINES,
        MULTIPLY_GRAPH,
        XOR_GRAPH,
        AND_GRAPH,
        OR_GRAPH,
    } background = Background::OR_GRAPH;
    Color primaryColor = Color(0x0000FF);
    Color secondaryColor = Color(0x008080);

    Desktop();
    virtual ~Desktop() = default;
    void Draw();
};
};

#endif
