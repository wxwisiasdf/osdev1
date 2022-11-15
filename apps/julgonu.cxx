/// julgonu.cxx
/// Implementation of Jul Gonu

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>


extern std::optional<UI::Desktop> g_Desktop;
__attribute__((section(".text.startup"))) int UDOS_32Main(char32_t[])
{
    struct Playfield
    {
        enum Cell
        {
            EMPTY = 0,
            BLACK = 1,
            WHITE = 2,
        };

        Playfield(size_t _cols, size_t _rows)
            : cols{ _cols },
            rows{ _rows }
        {
            this->cells.resize(this->cols * this->rows);
        }

        Cell& GetCell(size_t cx, size_t cy)
        {
            return this->cells[cx + cy * this->cols];
        }

        const Cell& GetCell(size_t cx, size_t cy) const
        {
            return this->cells[cx + cy * this->cols];
        }

        std::vector<Cell> cells, previousCells;
        size_t cols, rows;
    };
    static Playfield gamePlayfield(4, 4);

#define EMPTY Playfield::Cell::EMPTY
#define BLACK Playfield::Cell::BLACK
#define WHITE Playfield::Cell::WHITE
    gamePlayfield.cells = {
        WHITE, WHITE, WHITE, WHITE,
        EMPTY, EMPTY, EMPTY, EMPTY,
        EMPTY, EMPTY, EMPTY, EMPTY,
        BLACK, BLACK, BLACK, BLACK,
    };
#undef EMPTY
#undef BLACK
#undef WHITE

    auto& gameWindow = g_Desktop->AddChild<UI::Window>();
    gameWindow.SetText("Go but cheesy");

    auto& upBtn = gameWindow.AddChild<UI::Button>();
    upBtn.x = 12 * 0;
    upBtn.y = -16;
    upBtn.height = 12;
    upBtn.width = 12;
    upBtn.SetText("U");
    
    auto& downBtn = gameWindow.AddChild<UI::Button>();
    downBtn.x = 12 * 1;
    downBtn.y = -16;
    downBtn.height = 12;
    downBtn.width = 12;
    downBtn.SetText("D");

    auto& leftBtn = gameWindow.AddChild<UI::Button>();
    leftBtn.x = 12 * 2;
    leftBtn.y = -16;
    leftBtn.height = 12;
    leftBtn.width = 12;
    leftBtn.SetText("L");

    auto& rightBtn = gameWindow.AddChild<UI::Button>();
    rightBtn.x = 12 * 3;
    rightBtn.y = -16;
    rightBtn.height = 12;
    rightBtn.width = 12;
    rightBtn.SetText("R");

    struct PlayfieldWidget : UI::Viewport {
        virtual void Draw()
        {
            const auto drawSprite = [this](const int tpx, const int tpy, const int tile_idx)
            {
                constexpr size_t tw = 32; // Tile width and height
                constexpr size_t th = 32;
                const size_t to = tile_idx * tw * th;
                for(size_t tx = 0; tx < tw; tx++)
                {
                    for(size_t ty = 0; ty < th; ty++)
                    {
                        Color pixel{};
                        pixel.r = gameSpriteColorMap[gameSpriteData[to + tx + ty * tw]][2];
                        pixel.g = gameSpriteColorMap[gameSpriteData[to + tx + ty * tw]][1];
                        pixel.b = gameSpriteColorMap[gameSpriteData[to + tx + ty * tw]][0];
                        g_KFrameBuffer.PlotPixel(this->ox + (tpx * tw) + tx, this->oy + (tpy * th) + ty, pixel);
                    }
                }
            };

            // Draw our playfield
            for(size_t tx = 0; tx < gamePlayfield.cols; tx++)
            {
                for(size_t ty = 0; ty < gamePlayfield.rows; ty++)
                {
                    const auto& cell = gamePlayfield.cells[tx + ty * gamePlayfield.cols];
                    size_t spriteIdx = 0;
                    if(cell == Playfield::Cell::EMPTY || cell == Playfield::Cell::EMPTY)
                    {
                        if(tx == 0 && ty == 0)
                        {
                            spriteIdx = 3;
                        }
                        else if(tx == 0 && ty >= gamePlayfield.rows - 1)
                        {
                            spriteIdx = 2;
                        }
                        else if(tx >= gamePlayfield.cols - 1 && ty >= gamePlayfield.rows - 1)
                        {
                            spriteIdx = 1;
                        }
                        else if(tx >= gamePlayfield.cols - 1 && ty == 0)
                        {
                            spriteIdx = 4;
                        }
                        else if(tx == 0)
                        {
                            spriteIdx = 7;
                        }
                        else if(ty >= gamePlayfield.rows - 1)
                        {
                            spriteIdx = 6;
                        }
                        else if(ty == 0)
                        {
                            spriteIdx = 8;
                        }
                        else if(tx >= gamePlayfield.cols - 1)
                        {
                            spriteIdx = 9;
                        }
                    }
                    else if(cell == Playfield::Cell::BLACK)
                    {
                        spriteIdx = 10;
                    }
                    else if(cell == Playfield::Cell::WHITE)
                    {
                        spriteIdx = 11;
                    }
                    drawSprite(tx, ty, spriteIdx);
                }
            }
        }
    };

    auto& gameViewport = gameWindow.AddChild<PlayfieldWidget>();
    gameViewport.x = gameViewport.y = 0;
    gameViewport.width = 32 * gamePlayfield.cols;
    gameViewport.height = 32 * gamePlayfield.rows;
    gameViewport.OnClick = [](UI::Widget &w, unsigned int mx, unsigned int my, bool, bool)
    {
		static uint16_t randSeed = 0x5a7f;
        gamePlayfield.previousCells = gamePlayfield.cells;
        // TODO: Better turn system
        //if(!gamePlayfield.PlaceChip(mx >> 5, my >> 5, false))
        //    return;
        w.Redraw();
    };

    gameWindow.width = gameViewport.width + 12;
    gameWindow.height = gameViewport.height + 32;
    while (!gameWindow.isClosed)
        Task::Switch();
    return 0;
}
