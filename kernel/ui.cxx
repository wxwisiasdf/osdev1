#include <utility>
#include "ui.hxx"

#define MIN(x, y) ((x) > (y)) ? (y) : (x)
#define MAX(x, y) ((x) < (y)) ? (y) : (x)
#define CLAMP(x, min, max) MAX(MIN(x, max), min)

static UI::Manager ui_man(g_KFrameBuffer);

UI::Manager::Manager(Framebuffer &_fb)
    : fb{_fb}
{
}

UI::Manager &UI::Manager::Get()
{
    return ui_man;
}

void UI::Manager::SetFramebuffer(Framebuffer &_fb)
{
    this->fb = _fb;
}

/// @brief Draws the given widget (recursively)
/// @param w Widget to draw
/// @param ox Offset x to apply to widgets
/// @param oy Offset y to apply to widgets
void UI::Manager::Draw(UI::Widget &w, int ox, int oy)
{
    w.ox = w.x + ox;
    w.oy = w.y + oy;
    if (w.needs_redraw)
    {
#if 0
        this->drawn_x1 = MIN(this->drawn_x1, w.ox);
        this->drawn_y1 = MIN(this->drawn_y1, w.oy);
        this->drawn_x2 = MAX(this->drawn_x2, w.ox + w.width);
        this->drawn_y2 = MAX(this->drawn_y2, w.oy + w.height);
#endif
        w.Draw();
        w.needs_redraw = false;
        if (this->drag_widget == &w)
            return;
    }

    int extra_x = 0, extra_y = 0;
    // Draw in REVERSE order
    for (int i = MAX_CHILD_WIDGETS - 1; i >= 0; i--)
    {
        if (w.children[i])
        {
            auto &child = *w.children[i];
            // Flex effects only affects widgets that use the flex system
            if (child.flex != UI::Widget::Flex::NONE)
            {
                this->Draw(child, w.ox + w.padding_left + extra_x,
                           w.oy + w.padding_top + extra_y);
                if (child.flex == UI::Widget::Flex::ROW)
                    extra_x += child.width;
                else if (child.flex == UI::Widget::Flex::COLUMN)
                    extra_y += child.height;
            }
            else
                this->Draw(child, w.ox + w.padding_left,
                           w.oy + w.padding_top);
        }
    }
}

void UI::Manager::CheckRedraw(UI::Widget &, int, int)
{
#if 0
    w.ox = w.x + ox;
    w.oy = w.y + oy;
    for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
        if (w.children[i])
            this->CheckRedraw(*w.children[i], w.ox + w.padding_left,
                              w.oy + w.padding_top);
    
    // Redraw widget iff inside the drawn-over boundary box
    if (this->drawn_x2 >= w.ox && this->drawn_x1 <= w.ox + w.width
    && this->drawn_y2 >= w.oy && this->drawn_y1 <= w.oy + w.height)
        w.needs_redraw = true;
#endif
}

void UI::Manager::CheckUpdate(UI::Widget &w, unsigned mx, unsigned my, bool left, bool right, char32_t ch)
{
    w.inputted = false;
    if (w.OnUpdate)
        w.OnUpdate(w);
    
    if (this->drag_widget == &w) // Drag widget around
    {
        if (!left) // Stop drag
        {
            TTY::Print("ui: Stop drag\n");
            this->drag_widget->SetSkeleton(false);
            this->drag_widget->Redraw();
            this->drag_widget = nullptr;

            if (w.parent)
                w.parent->Redraw();
        }
        else // Continue dragging
        {
            w.MoveBy(mx - this->drag_x, my - this->drag_y);
            if (w.parent)
            {
                // Make sure widget is inside bounds
                w.ox = CLAMP(w.ox, w.parent->ox + (int)w.width, w.parent->ox + (int)w.parent->width - (int)w.width);
                w.oy = CLAMP(w.oy, w.parent->oy + (int)w.height, w.parent->oy + (int)w.parent->height - (int)w.height);
            }

            this->drag_x = mx;
            this->drag_y = my;
            this->drag_widget->SetSkeleton(true); // Do not redraw wholly when moving
        }
        return;
    }

    // We must be actually hovering this widget
    if (static_cast<signed short>(mx) >= w.ox && static_cast<signed short>(mx) <= w.ox + static_cast<signed short>(w.width) && static_cast<signed short>(my) >= w.oy && static_cast<signed short>(my) <= w.oy + static_cast<signed short>(w.height))
    {
        // Evaluate children iff we're hovering over
        for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
        {
            if (w.children[i])
            {
                auto &child = *w.children[i];
                this->CheckUpdate(child, mx, my, left, right, ch);
                if (child.clicked)
                    return;
            }
        }

        // Input is only evaluated iff needed and we return immediately
        if (w.OnInput && ch > U'\0')
        {
            w.OnInput(w, ch);
            w.inputted = true;
        }

        if (left || right)
        {
            if (left && w.dragable)
            { // Drag the widget
                this->drag_x = mx;
                this->drag_y = my;
                this->drag_widget = &w;
                if (w.parent) // If we have a parent, put us at the mere front
                {
                    if (w.parent->children[0] != this->drag_widget)
                    {
                        for (size_t i = 1; i < MAX_CHILD_WIDGETS; i++)
                        {
                            if (w.parent->children[i] == this->drag_widget)
                            {
                                std::swap(w.parent->children[0], w.parent->children[i]);
                                w.parent->children[0] = this->drag_widget;
                                w.parent->children[0]->Redraw();
                                break;
                            }
                        }
                        w.parent->Redraw();
                    }
                }
                w.clicked = true;
            }
            else if (w.OnClick && !w.clicked)
            {
                w.OnClick(w, mx - w.ox, my - w.oy, left, right);
                w.clicked = true;
            }
        }
        else
        {
            w.clicked = false;
            if (!w.tooltipHovered && (w.tooltipText != nullptr || w.tooltipText32 != nullptr))
            {
                int tooltip_x = w.ox, tooltip_y = w.oy + w.height;
                if (tooltip_x + w.width < this->fb.width)
                    tooltip_x += w.width;

                int fx, fy;
                bool multiline;
                if (w.tooltipText32 != nullptr)
                {
                    w.DrawText(w.tooltipText32, tooltip_x, tooltip_y, 128, this->fb.height - tooltip_y, fx, fy, multiline, Color(0x000000));
                    g_KFrameBuffer.FillSquare(tooltip_x, tooltip_y, fx - tooltip_x, fy - tooltip_y, Color(0xFFFF00));
                    w.DrawText(w.tooltipText32, tooltip_x, tooltip_y, 128, this->fb.height - tooltip_y, fx, fy, multiline, Color(0x000000));
                }
                else if (w.tooltipText != nullptr)
                {
                    w.DrawText(w.tooltipText, tooltip_x, tooltip_y, 128, this->fb.height - tooltip_y, fx, fy, multiline, Color(0x000000));
                    g_KFrameBuffer.FillSquare(tooltip_x, tooltip_y, fx - tooltip_x, fy - tooltip_y, Color(0xFFFF00));
                    w.DrawText(w.tooltipText, tooltip_x, tooltip_y, 128, this->fb.height - tooltip_y, fx, fy, multiline, Color(0x000000));
                }
                w.tooltipHovered = true;
            }

            if (w.OnHover) // Only hover/click 1 at a time
                w.OnHover(w, mx - w.ox, my - w.oy);
        }
    }
    else
    {
        // Recursively set the tooltip status for all children to false
        const auto setTooltipFalse = [](auto& w) -> void {
            auto setTooltipFalse_impl = [](auto& w, const auto& func) -> void {
                w.tooltipHovered = false;
                for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
                    if (w.children[i])
                        func(*w.children[i], func);
            };
            setTooltipFalse_impl(w, setTooltipFalse_impl);
        };
        setTooltipFalse(w);
    }
}

/// @brief Updates the manager state to prepare for the next frame
void UI::Manager::Update()
{
    this->drawn_x1 = 0;
    this->drawn_y1 = 0;
    this->drawn_x2 = 0xffff;
    this->drawn_y2 = 0xffff;
}

UI::Widget::Widget()
{
}

UI::Widget::~Widget()
{
    if (this->parent) {
        for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
            if (this->parent->children[i] == this)
                this->parent->children[i] = nullptr;
        // TODO: Assert we need redraw at all
        this->parent->Redraw();
    }
}

void UI::Widget::SetText(const char *text)
{
    this->text = text;
    this->Redraw();
}

void UI::Widget::SetText(const char32_t *text)
{
    this->text32 = text;
    this->Redraw();
}

const char *UI::Widget::GetText() const
{
    return this->text == nullptr ? "" : this->text;
}

void UI::Widget::SetTooltipText(const char* text) {
    this->tooltipText = text; // No redraw required
}

void UI::Widget::SetTooltipText(const char32_t* text) {
    this->tooltipText32 = text; // No redraw required
}

const char *UI::Widget::GetTooltipText() const {
    return this->tooltipText == nullptr ? "" : this->tooltipText;
}

void UI::Widget::AddChild(UI::Widget &w)
{
    for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
    {
        if (this->children[i] == nullptr)
        {
            this->children[i] = &w;
            this->n_children++;
            w.parent = this;
            return;
        }
    }
    // Get fucked, no child added
}

void UI::Widget::DrawShadow(Color outer_color, Color inner_color, Color mid_color)
{
    // -- Black bottom shadow
    // Bottom left to bottom right
    g_KFrameBuffer.PlotLine(this->ox, this->oy + this->height - 1,
                            this->ox + this->width - 1,
                            this->oy + this->height - 1,
                            inner_color);
    // Bottom right to top right
    g_KFrameBuffer.PlotLine(this->ox + this->width - 1,
                            this->oy + this->height - 1,
                            this->ox + this->width - 1,
                            this->oy,
                            inner_color);

    // -- Light grey shadow
    // Bottom left to bottom right
    g_KFrameBuffer.PlotLine(this->ox + 1,
                            this->oy + this->height - 2,
                            this->ox + this->width - 2,
                            this->oy + this->height - 2,
                            mid_color);
    // Bottom right to top right
    g_KFrameBuffer.PlotLine(this->ox + this->width - 2,
                            this->oy + this->height - 2,
                            this->ox + this->width - 2,
                            this->oy + 1, mid_color);

    // -- White reflection at top
    // From bottom to top left
    g_KFrameBuffer.PlotLine(this->ox + 1,
                            this->oy + this->height - 2,
                            this->ox + 1,
                            this->oy + 1, outer_color);
    // From left to right top
    g_KFrameBuffer.PlotLine(this->ox + 1,
                            this->oy + 1,
                            this->ox + this->width - 2,
                            this->oy + 1, outer_color);
}

/// @brief Force a redraw of the given widget
void UI::Widget::Redraw()
{
    this->needs_redraw = true;
    for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
        if (this->children[i])
            this->children[i]->Redraw();
}

void UI::Widget::SetSkeleton(bool value)
{
    this->skeleton = value;
    for (size_t i = 0; i < MAX_CHILD_WIDGETS; i++)
        if (this->children[i])
            this->children[i]->SetSkeleton(value);
}

void UI::Widget::MoveBy(int off_x, int off_y)
{
    this->x += off_x;
    this->y += off_y;
    this->Redraw();
}

static void PerformDrawText(const auto* text, int tx, int ty, int max_x, int max_y, int& final_x, int& final_y, bool& multiline, Color color)
{
    final_y = final_x = 0;
    multiline = false;
    
    int base_x = tx;
    int base_y = ty;
    const auto* p = text;
    for (size_t i = 0; p[i]; i++)
    {
        if (p[i] == '\n')
        {
            tx = base_x;
            ty += 8;
            multiline = true;
            if (ty >= base_y + static_cast<decltype(base_y)>(max_y))
                break;
        }
        else if (p[i] == '\r')
        {
            tx = base_x;
        }
        else if (p[i] == '\v')
        {
            ty++;
            multiline = true;
        }

        g_KFrameBuffer.PlotChar(tx, ty, p[i], color);
        tx += g_KFrameBuffer.advance_x;
        if (tx > final_x)
            final_x = tx;
        ty += g_KFrameBuffer.advance_y;
        if (tx >= base_x + static_cast<decltype(base_x)>(max_x))
        {
            tx = base_x;
            ty += 8;
            multiline = true;
        }
    }
    final_y = ty + 8;
}

void UI::Widget::DrawText(const char* text, int tx, int ty, int max_x, int max_y, int& final_x, int& final_y, bool& multiline, Color color)
{
    PerformDrawText(text, tx, ty, max_x, max_y, final_x, final_y, multiline, color);
}

void UI::Widget::DrawText(const char32_t* text, int tx, int ty, int max_x, int max_y, int& final_x, int& final_y, bool& multiline, Color color)
{
    PerformDrawText(text, tx, ty, max_x, max_y, final_x, final_y, multiline, color);
}

void UI::Widget::DrawText(int tx, int ty, Color color)
{
    int fx, fy;
    bool multiline;

    if (this->text32 != nullptr)
    {
        this->DrawText(this->text32, tx, ty, this->width, this->height, fx, fy, multiline, color);
    }
    else if (this->text != nullptr)
    {
        this->DrawText(this->text, tx, ty, this->width, this->height, fx, fy, multiline, color);
    }
}

void UI::Widget::Draw()
{
    this->DrawText(this->ox + 4, this->oy + 4, Color(0x000000));
}

UI::Textbox::Textbox()
    : UI::Widget()
{
    this->textBuffer[0] = U'\0';
    this->bufferPos = 0;
    this->OnInput = [](UI::Widget &w, char32_t keypress) {
        auto& o = static_cast<UI::Textbox&>(w);
        if (o.bufferPos >= sizeof(o.textBuffer) - 1)
            return;
        
        if (keypress == '\n' || keypress == '\r')
        {
            // Ignore
            return;
        }
        // Backspace
        else if (keypress == '\b')
        {
            if (o.bufferPos > 0)
            {
                o.textBuffer[--o.bufferPos] = U'\0';
            }
            // TODO: Do stuff
            o.SetText(o.textBuffer);
        }
        else
        {
            o.textBuffer[o.bufferPos++] = keypress;
            o.textBuffer[o.bufferPos] = U'\0';
            o.SetText(o.textBuffer);
        }
    };
}

void UI::Textbox::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0xFFFFFF));
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    this->DrawText(this->ox + this->padding_left, this->oy + this->padding_top, Color(0x000000));
}

void UI::Button::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0xA0A0A0));
    this->DrawShadow(Color(0xFFFFFF), Color(0x000000), Color(0x808080));
    this->DrawText(this->ox + this->padding_left, this->oy + this->padding_top, Color(0x000000));
}

UI::Group::Group()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 4;
    this->padding_left = 4;
}

void UI::Group::Draw()
{
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    this->DrawText(this->ox + 4, this->oy + 4, Color(0x000000));
}

UI::Menu::Menu()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 4;
    this->padding_left = 4;
}

void UI::Menu::Draw()
{
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    this->DrawText(this->ox + 4, this->oy + 4, Color(0x000000));
}

UI::Taskbar::Taskbar()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 4;
    this->padding_left = 4;
}

void UI::Taskbar::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0xA0A0A0));
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    this->DrawText(this->ox + 4, this->oy + 4, Color(0x000000));
}

UI::Window::Window()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 24 + 4;
    this->padding_left = 4;
    this->dragable = true;
}

void UI::Window::Decorate()
{
    this->closeBtn.emplace();
    this->closeBtn->y = -16;
    this->closeBtn->x = 0;
    this->closeBtn->width = this->closeBtn->height = 12;
    this->closeBtn->SetText("X");
    this->closeBtn->OnClick = ([](UI::Widget &w, unsigned, unsigned, bool,
                                  bool) -> void
                               { static_cast<UI::Window &>(*w.parent).isClosed = true; });
    this->AddChild(this->closeBtn.value());

    this->minimBtn.emplace();
    this->minimBtn->y = -16;
    this->minimBtn->x = 14;
    this->minimBtn->width = this->minimBtn->height = 12;
    this->minimBtn->SetText("-");
    this->minimBtn->OnClick = ([](UI::Widget &w, unsigned, unsigned, bool,
                                  bool) -> void
                               { static_cast<UI::Window &>(*w.parent).isClosed = true; });
    this->AddChild(this->minimBtn.value());
}

void UI::Window::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0xA0A0A0));
    
    for (size_t i = 0; i < 12; i++) {
        const auto b = (0xFF - i * 19) & 0xFF;
        g_KFrameBuffer.PlotLine(this->ox + 1, this->oy + 1 + i, this->ox + this->width - 1, this->oy + 1 + i, Color((b << 24) | (b << 16) | (b << 8) | 0xFF));
    }

    this->DrawShadow(Color(0xFFFFFF), Color(0x000000), Color(0x808080));
    this->DrawText(this->ox + 4, this->oy + 2, Color(0xFFFFFF));
}

UI::Viewport::Viewport()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 4;
    this->padding_left = 4;
}

void UI::Viewport::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0x000000));
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    if (this->OnDraw)
        this->OnDraw(*this, this->ox, this->oy, this->ox + this->width, this->oy + this->height);
    this->DrawText(this->ox + 4, this->oy + 4, Color(0xFFFFFF));
}

UI::Terminal::Terminal()
    : UI::Widget()
{
    this->padding_bottom = 4;
    this->padding_right = 4;
    this->padding_top = 4;
    this->padding_left = 4;
    // Update accordingly if the terminal has updated
    this->OnUpdate = ([](UI::Widget& w) {
        auto& o = static_cast<UI::Terminal&>(w);
        if(o.term.modified) {
            o.Redraw();
            o.term.modified = false;
        }
    });
}

void UI::Terminal::Draw()
{
    if (!this->skeleton)
        g_KFrameBuffer.FillSquare(this->ox, this->oy, this->width, this->height, Color(0x000000));
    this->DrawShadow(Color(0x000000), Color(0xFFFFFF), Color(0x808080));
    this->term.GetBuffer()[MAX_TERM_BUF - 1] = '\0';
    this->SetText(this->term.GetBuffer());
    this->DrawText(this->ox + 4, this->oy + 4, Color(0xFFFFFF));
}

UI::DesktopIcon::DesktopIcon()
    : UI::Widget()
{
    this->padding_bottom = 0;
    this->padding_right = 0;
    this->padding_top = 0;
    this->padding_left = 0;
}

void UI::DesktopIcon::Draw()
{

}

UI::Desktop::Desktop()
    : UI::Widget()
{
    this->padding_bottom = 0;
    this->padding_right = 0;
    this->padding_top = 0;
    this->padding_left = 0;
}

void UI::Desktop::Draw()
{
    switch (background)
    {
    case Background::SOLID:
        for (size_t i = 0; i < this->width; i++)
            for (size_t j = 0; j < this->height; j++)
                g_KFrameBuffer.PlotPixel(i, j, this->primaryColor);
        break;
    case Background::DIAGONAL_LINES:
        for (size_t i = 0; i < this->width; i++)
            for (size_t j = 0; j < this->height; j++)
                g_KFrameBuffer.PlotPixel(i, j, Color(this->primaryColor.rgba + i + j));
        break;
    case Background::MULTIPLY_OFFSET:
        for (size_t i = 0; i < this->width; i++)
            for (size_t j = 0; j < this->height; j++)
                g_KFrameBuffer.PlotPixel(i, j, Color(this->primaryColor.rgba + i * j));
        break;
    }
}
