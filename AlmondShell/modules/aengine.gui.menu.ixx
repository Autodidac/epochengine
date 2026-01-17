
module; // REQUIRED global module fragment

#include "aengine.config.hpp" 		// for ALMOND_USING_SDL
export module aengine.gui.menu;

// ------------------------------------------------------------
// Engine headers (header units, order-sensitive)
// ------------------------------------------------------------

//import aengine.config;

import aengine.core.context;
import aengine.context.multiplexer;
import aengine.cli;
import aengine.input;
import aengine.gui;
import aengine.context.window;
import aengine.core.context;
import aengine.context.type;

// ------------------------------------------------------------
// Standard library
// ------------------------------------------------------------
import <vector>;
import <string>;
import <optional>;
import <tuple>;
import <algorithm>;
import <array>;
import <cmath>;
import <iomanip>;
import <iostream>;
import <memory>;
import <sstream>;

// ============================================================
// Menu
// ============================================================

namespace gui = almondnamespace::gui;

export namespace almondnamespace::menu
{
    enum class Choice {
        Snake, Tetris, Pacman, Sokoban,
        Minesweep, Puzzle, Bejeweled, Fourty,
        Sandsim, Cellular, Settings, Exit
    };

    struct ChoiceDescriptor {
        Choice      choice;
        std::string label;
        gui::Vec2   size;
    };

    struct MenuOverlay
    {
        std::vector<ChoiceDescriptor> descriptors;
        std::size_t selection = 0;

        bool prevUp = false, prevDown = false, prevLeft = false,
            prevRight = false, prevEnter = false;
        bool initialized = false;

        std::vector<std::pair<int, int>> cachedPositions;
        std::vector<float> colWidths, rowHeights;

        int cachedWidth = -1;
        int cachedHeight = -1;
        int columns = 1;
        int rows = 0;

        static constexpr int ExpectedColumns = 4;
        static constexpr float LayoutSpacing = 32.f;

        int maxColumns = ExpectedColumns;

        float layoutOriginX = 0.0f;
        float layoutOriginY = 0.0f;
        float layoutWidth = 0.0f;
        float layoutHeight = 0.0f;

        // ----------------------------------------------------
        void set_max_columns(int desiredMax)
        {
            const int clamped = std::clamp(desiredMax, 1, ExpectedColumns);
            if (maxColumns != clamped) {
                maxColumns = clamped;
                cachedWidth = -1;
                cachedHeight = -1;
            }
        }

        // ----------------------------------------------------
        void recompute_layout(
            std::shared_ptr<almondnamespace::core::Context> ctx,
            int widthPixels,
            int heightPixels)
        {
            const int totalItems = static_cast<int>(descriptors.size());
            if (totalItems == 0) {
                cachedPositions.clear();
                colWidths.clear();
                rowHeights.clear();
                columns = 1;
                rows = 0;
                layoutOriginX = layoutOriginY = 0.0f;
                layoutWidth = layoutHeight = 0.0f;
                return;
            }

            int resolvedWidth = widthPixels;
            int resolvedHeight = heightPixels;

            if (resolvedWidth <= 0 && ctx)  resolvedWidth = ctx->get_width_safe();
            if (resolvedHeight <= 0 && ctx) resolvedHeight = ctx->get_height_safe();

            cachedWidth = (std::max)(1, resolvedWidth);
            cachedHeight = (std::max)(1, resolvedHeight);

            float maxItemWidth = 0.f;
            float maxItemHeight = 0.f;
            for (const auto& d : descriptors) {
                maxItemWidth = (std::max)(maxItemWidth, d.size.x);
                maxItemHeight = (std::max)(maxItemHeight, d.size.y);
            }

            int computedCols = totalItems;
            if (maxItemWidth > 0.f) {
                const float available = static_cast<float>(cachedWidth);
                const float denom = maxItemWidth + LayoutSpacing;
                if (denom > 0.f) {
                    computedCols = static_cast<int>(
                        std::floor((available + LayoutSpacing) / denom));
                    computedCols = std::clamp(computedCols, 1, totalItems);
                }
            }

            const int maxAllowed = (std::max)(1, (std::min)(totalItems, maxColumns));
            columns = (std::max)(1, (std::min)(computedCols, maxAllowed));
            rows = (totalItems + columns - 1) / columns;

            colWidths.assign(columns, 0.f);
            rowHeights.assign(rows, 0.f);

            for (int i = 0; i < totalItems; ++i) {
                const int r = i / columns;
                const int c = i % columns;
                colWidths[c] = (std::max)(colWidths[c], descriptors[i].size.x);
                rowHeights[r] = (std::max)(rowHeights[r], descriptors[i].size.y);
            }

            float totalWidth = LayoutSpacing * (columns - 1);
            float totalHeight = LayoutSpacing * (rows - 1);
            for (float w : colWidths)  totalWidth += w;
            for (float h : rowHeights) totalHeight += h;

            layoutOriginX = (std::max)(0.f, (cachedWidth - totalWidth) * 0.5f);
            layoutOriginY = (std::max)(0.f, (cachedHeight - totalHeight) * 0.5f);
            layoutWidth = totalWidth;
            layoutHeight = totalHeight;

            cachedPositions.resize(totalItems);

            float y = layoutOriginY;
            for (int r = 0; r < rows; ++r) {
                float x = layoutOriginX;
                for (int c = 0; c < columns; ++c) {
                    const int idx = r * columns + c;
                    if (idx < totalItems) {
                        cachedPositions[idx] = {
                            static_cast<int>(std::round(x)),
                            static_cast<int>(std::round(y))
                        };
                    }
                    x += colWidths[c] + LayoutSpacing;
                }
                y += rowHeights[r] + LayoutSpacing;
            }
        }

        // ----------------------------------------------------
        void initialize(std::shared_ptr<core::Context> ctx)
        {
            if (initialized) return;

            set_max_columns(core::cli::menu_columns);

            selection = 0;
            prevUp = prevDown = prevLeft = prevRight = prevEnter = false;

            constexpr gui::Vec2 DefaultButtonSize{ 256.0f, 96.0f };

            descriptors = {
                { Choice::Snake,      "Snake",      DefaultButtonSize },
                { Choice::Tetris,    "Tetris",     DefaultButtonSize },
                { Choice::Pacman,    "Pacman",     DefaultButtonSize },
                { Choice::Sokoban,   "Sokoban",    DefaultButtonSize },
                { Choice::Minesweep, "Minesweep",  DefaultButtonSize },
                { Choice::Puzzle,    "Puzzle",     DefaultButtonSize },
                { Choice::Bejeweled, "Bejeweled",  DefaultButtonSize },
                { Choice::Fourty,    "2048",       DefaultButtonSize },
                { Choice::Sandsim,   "Sand Sim",   DefaultButtonSize },
                { Choice::Cellular,  "Cellular",   DefaultButtonSize },
                { Choice::Settings,  "Settings",   DefaultButtonSize },
                { Choice::Exit,      "Quit",       DefaultButtonSize }
            };

            const int w = ctx ? ctx->get_width_safe() : cachedWidth;
            const int h = ctx ? ctx->get_height_safe() : cachedHeight;
            recompute_layout(ctx, w, h);

            initialized = true;
            std::cout << "[Menu] Initialized " << descriptors.size() << " entries\n";
        }

        // ----------------------------------------------------
        std::optional<Choice> update_and_draw(
            std::shared_ptr<core::Context> ctx,
            core::WindowData* win,
            float dt,
            bool upPressed,
            bool downPressed,
            bool leftPressed,
            bool rightPressed,
            bool enterPressed)
        {
            if (!initialized) return std::nullopt;

            std::ignore = win;
            std::ignore = dt;

            int currentWidth = ctx ? ctx->get_width_safe() : cachedWidth;
            int currentHeight = ctx ? ctx->get_height_safe() : cachedHeight;
            if (currentWidth <= 0) currentWidth = cachedWidth;
            if (currentHeight <= 0) currentHeight = cachedHeight;
            if (currentWidth <= 0) currentWidth = 1;
            if (currentHeight <= 0) currentHeight = 1;

            if (currentWidth != cachedWidth || currentHeight != cachedHeight)
                recompute_layout(ctx, currentWidth, currentHeight);

            int mx = 0, my = 0;
            ctx->get_mouse_position_safe(mx, my);

            const int totalItems = int(descriptors.size());
            if (totalItems == 0 || cachedPositions.size() != size_t(totalItems))
                return std::nullopt;

            if (selection >= size_t(totalItems))
                selection = size_t(totalItems - 1);

           // const bool flipVertical = ctx && ctx ->type == core::ContextType::OpenGL;

            auto position_for_index = [&](int idx) {
                return cachedPositions[idx];
                };

            int hover = -1;
            for (int i = 0; i < totalItems; ++i) {
                const auto& d = descriptors[i];
                const auto pos = position_for_index(i);
                if (mx >= pos.first && mx <= pos.first + int(d.size.x) &&
                    my >= pos.second && my <= pos.second + int(d.size.y)) {
                    hover = i;
                    break;
                }
            }

            const int cols = (std::max)(1, columns);
            const int rowsLocal = (std::max)(1, rows);

            if (leftPressed && !prevLeft)  selection = (selection == 0) ? totalItems - 1 : selection - 1;
            if (rightPressed && !prevRight) selection = (selection + 1) % totalItems;
            if (upPressed && !prevUp)    selection = (selection < cols) ? selection + (rowsLocal - 1) * cols : selection - cols;
            if (downPressed && !prevDown)  selection = (selection + cols >= totalItems) ? selection % cols : selection + cols;
            if (!upPressed && !downPressed && !leftPressed && !rightPressed && hover >= 0)
                selection = hover;

            prevUp = upPressed; prevDown = downPressed;
            prevLeft = leftPressed; prevRight = rightPressed;

            const float pad = LayoutSpacing * 0.5f;
            gui::begin_window(
                "Main Menu",
                { layoutOriginX - pad, layoutOriginY - pad },
                { layoutWidth + pad * 2, layoutHeight + pad * 2 }
            );

            std::optional<Choice> chosen{};
            for (int i = 0; i < totalItems; ++i) {
                const auto pos = position_for_index(i);
                gui::set_cursor({ float(pos.first), float(pos.second) });

                std::string label = descriptors[i].label;
                if (size_t(i) == selection) label = "> " + label + " <";

                if (gui::button(label, descriptors[i].size)) {
                    selection = size_t(i);
                    chosen = descriptors[i].choice;
                }
            }

            gui::end_window();

            if (chosen) return chosen;
            if (enterPressed && !prevEnter) return Choice(selection);

            prevEnter = enterPressed;
            return std::nullopt;
        }

        // ----------------------------------------------------
        void cleanup()
        {
            descriptors.clear();
            cachedPositions.clear();
            colWidths.clear();
            rowHeights.clear();
            cachedWidth = cachedHeight = -1;
            columns = 1;
            rows = 0;
            layoutOriginX = layoutOriginY = 0.f;
            layoutWidth = layoutHeight = 0.f;
            selection = 0;
            prevUp = prevDown = prevLeft = prevRight = prevEnter = false;
            initialized = false;
        }
    };
}
