export module aengine.gui.menu;

import <algorithm>;
import <array>;
import <bitset>;
import <cmath>;
import <cstddef>;
import <functional>;
import <iomanip>;
import <iostream>;
import <memory>;
import <optional>;
import <sstream>;
import <string>;
import <tuple>;
import <utility>;
import <vector>;

// Engine modules (replace these with your actual module names)
import aengine.platform;             // replaces aplatform.hpp / aengineconfig.hpp include soup
import aengine.types;                // replaces atypes.hpp
import aengine.context;              // replaces acontext.hpp
import aengine.contextmultiplexer;   // replaces acontextmultiplexer.hpp
import aengine.commandline;          // replaces acommandline.hpp
import aengine.input;                // replaces ainput.hpp
import aengine.gui;                  // replaces agui.hpp

export namespace almondnamespace::menu
{
    export enum class Choice
    {
        Snake, Tetris, Pacman, Sokoban,
        Minesweep, Puzzle, Bejeweled, Fourty,
        Sandsim, Cellular, Settings, Exit
    };

    export struct ChoiceDescriptor
    {
        Choice      choice{};
        std::string label{};
        gui::Vec2   size{};
    };

    export struct MenuOverlay
    {
        std::vector<ChoiceDescriptor> descriptors{};
        std::size_t selection{ 0 };
        bool prevUp{ false }, prevDown{ false }, prevLeft{ false }, prevRight{ false }, prevEnter{ false };
        bool initialized{ false };

        std::vector<std::pair<int, int>> cachedPositions{}; // x,y per item
        std::vector<float> colWidths{};
        std::vector<float> rowHeights{};
        int cachedWidth{ -1 };
        int cachedHeight{ -1 };
        int columns{ 1 };
        int rows{ 0 };
        int maxColumns{ ExpectedColumns };

        float layoutOriginX{ 0.0f };
        float layoutOriginY{ 0.0f };
        float layoutWidth{ 0.0f };
        float layoutHeight{ 0.0f };

        static constexpr int   ExpectedColumns = 4;
        static constexpr float LayoutSpacing = 32.0f;

        export void set_max_columns(int desiredMax);
        export void recompute_layout(std::shared_ptr<core::Context> ctx, int widthPixels, int heightPixels);
        export void initialize(std::shared_ptr<core::Context> ctx);

        export std::optional<Choice> update_and_draw(
            std::shared_ptr<core::Context> ctx,
            core::WindowData* win,
            float dt,
            bool upPressed,
            bool downPressed,
            bool leftPressed,
            bool rightPressed,
            bool enterPressed);

        export void cleanup();
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Implementation section (still in the same .ixx file; no .cpp needed)
// ─────────────────────────────────────────────────────────────────────────────
module aengine.menu;

namespace almondnamespace::menu
{
    void MenuOverlay::set_max_columns(int desiredMax)
    {
        const int clamped = std::clamp(desiredMax, 1, ExpectedColumns);
        if (maxColumns != clamped)
        {
            maxColumns = clamped;
            cachedWidth = -1;
            cachedHeight = -1;
        }
    }

    void MenuOverlay::recompute_layout(std::shared_ptr<core::Context> ctx, int widthPixels, int heightPixels)
    {
        const int totalItems = static_cast<int>(descriptors.size());
        if (totalItems == 0)
        {
            cachedPositions.clear();
            colWidths.clear();
            rowHeights.clear();
            columns = 1;
            rows = 0;
            layoutOriginX = 0.0f;
            layoutOriginY = 0.0f;
            layoutWidth = 0.0f;
            layoutHeight = 0.0f;
            return;
        }

        int resolvedWidth = widthPixels;
        int resolvedHeight = heightPixels;
        if (resolvedWidth <= 0 && ctx) resolvedWidth = ctx->get_width_safe();
        if (resolvedHeight <= 0 && ctx) resolvedHeight = ctx->get_height_safe();
        cachedWidth = (std::max)(1, resolvedWidth);
        cachedHeight = (std::max)(1, resolvedHeight);

        float maxItemWidth = 0.0f;
        float maxItemHeight = 0.0f;
        for (const auto& d : descriptors)
        {
            maxItemWidth = (std::max)(maxItemWidth, d.size.x);
            maxItemHeight = (std::max)(maxItemHeight, d.size.y);
        }

        const float spacing = LayoutSpacing;

        int computedCols = totalItems;
        if (maxItemWidth > 0.0f)
        {
            const float availableWidth = static_cast<float>((std::max)(1, cachedWidth));
            const float denom = maxItemWidth + spacing;
            if (denom > 0.0f)
            {
                computedCols = static_cast<int>(std::floor((availableWidth + spacing) / denom));
                computedCols = std::clamp(computedCols, 1, totalItems);
            }
        }

        const int maxAllowed = (std::max)(1, (std::min)(totalItems, maxColumns));
        computedCols = (std::min)(computedCols, maxAllowed);
        columns = (std::max)(1, computedCols);
        rows = (totalItems + columns - 1) / columns;

        colWidths.assign(columns, 0.0f);
        rowHeights.assign(rows, 0.0f);
        for (int idx = 0; idx < totalItems; ++idx)
        {
            const int r = idx / columns;
            const int c = idx % columns;
            const auto& d = descriptors[idx];
            colWidths[c] = (std::max)(colWidths[c], d.size.x);
            rowHeights[r] = (std::max)(rowHeights[r], d.size.y);
        }

        float totalWidth = spacing * (std::max)(0, columns - 1);
        for (float w : colWidths) totalWidth += w;

        float totalHeight = spacing * (std::max)(0, rows - 1);
        for (float h : rowHeights) totalHeight += h;

        const float baseX = (std::max)(0.0f, (static_cast<float>(cachedWidth) - totalWidth) * 0.5f);
        const float baseY = (std::max)(0.0f, (static_cast<float>(cachedHeight) - totalHeight) * 0.5f);

        layoutOriginX = baseX;
        layoutOriginY = baseY;
        layoutWidth = totalWidth;
        layoutHeight = totalHeight;

        cachedPositions.resize(totalItems);

        float yPos = baseY;
        for (int r = 0; r < rows; ++r)
        {
            float xPos = baseX;
            for (int c = 0; c < columns; ++c)
            {
                const int idx = r * columns + c;
                if (idx < totalItems)
                {
                    cachedPositions[idx] = {
                        static_cast<int>(std::round(xPos)),
                        static_cast<int>(std::round(yPos))
                    };
                }
                xPos += colWidths[c] + spacing;
            }
            yPos += rowHeights[r] + spacing;
        }
    }

    void MenuOverlay::initialize(std::shared_ptr<core::Context> ctx)
    {
        if (initialized) return;

        set_max_columns(core::cli::menu_columns);

        selection = 0;
        prevUp = prevDown = prevLeft = prevRight = prevEnter = false;

        constexpr gui::Vec2 DefaultButtonSize{ 256.0f, 96.0f };

        descriptors = {
            { Choice::Snake,     "Snake",     DefaultButtonSize },
            { Choice::Tetris,    "Tetris",    DefaultButtonSize },
            { Choice::Pacman,    "Pacman",    DefaultButtonSize },
            { Choice::Sokoban,   "Sokoban",   DefaultButtonSize },
            { Choice::Minesweep, "Minesweep", DefaultButtonSize },
            { Choice::Puzzle,    "Puzzle",    DefaultButtonSize },
            { Choice::Bejeweled, "Bejeweled", DefaultButtonSize },
            { Choice::Fourty,    "2048",      DefaultButtonSize },
            { Choice::Sandsim,   "Sand Sim",  DefaultButtonSize },
            { Choice::Cellular,  "Cellular",  DefaultButtonSize },
            { Choice::Settings,  "Settings",  DefaultButtonSize },
            { Choice::Exit,      "Quit",      DefaultButtonSize }
        };

        const int w = ctx ? ctx->get_width_safe() : cachedWidth;
        const int h = ctx ? ctx->get_height_safe() : cachedHeight;
        recompute_layout(ctx, w, h);

        initialized = true;
        std::cout << "[Menu] Initialized " << descriptors.size() << " entries\n";
    }

    std::optional<Choice> MenuOverlay::update_and_draw(
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

        if (currentWidth <= 0 && cachedWidth > 0) currentWidth = cachedWidth;
        if (currentHeight <= 0 && cachedHeight > 0) currentHeight = cachedHeight;
        if (currentWidth <= 0) currentWidth = 1;
        if (currentHeight <= 0) currentHeight = 1;

        if (currentWidth != cachedWidth || currentHeight != cachedHeight)
            recompute_layout(ctx, currentWidth, currentHeight);

        int mx = 0, my = 0;
        if (ctx) ctx->get_mouse_position_safe(mx, my);

        const int totalItems = static_cast<int>(descriptors.size());
        if (totalItems == 0 || cachedPositions.size() != static_cast<std::size_t>(totalItems))
            return std::nullopt;

        if (selection >= static_cast<std::size_t>(totalItems))
            selection = static_cast<std::size_t>(totalItems - 1);

        const bool flipVertical = ctx && ctx->type == core::ContextType::OpenGL;

        std::vector<int> rowBaseY;
        std::vector<int> colBaseX;

        if (flipVertical)
        {
            rowBaseY.resize((std::max)(1, rows));
            for (int r = 0; r < rows; ++r)
            {
                const int idx = r * columns;
                if (idx < totalItems)
                    rowBaseY[r] = cachedPositions[idx].second;
                else if (r > 0)
                    rowBaseY[r] = rowBaseY[r - 1];
                else
                    rowBaseY[r] = 0;
            }

            colBaseX.resize((std::max)(1, columns));
            for (int c = 0; c < columns; ++c)
            {
                if (c < totalItems)
                    colBaseX[c] = cachedPositions[c].first;
                else if (!cachedPositions.empty())
                    colBaseX[c] = cachedPositions.back().first;
                else
                    colBaseX[c] = 0;
            }
        }

        auto position_for_index = [&](int idx) -> std::pair<int, int>
            {
                if (idx < 0 || idx >= totalItems)
                    return { 0, 0 };

                if (!flipVertical || rowBaseY.empty() || colBaseX.empty())
                    return cachedPositions[idx];

                const int r = idx / columns;
                const int c = idx % columns;
                const int flippedRow = std::clamp(rows - 1 - r, 0, rows - 1);
                const int clampedCol = std::clamp(c, 0, columns - 1);
                return { colBaseX[clampedCol], rowBaseY[flippedRow] };
            };

        int hover = -1;
        for (int i = 0; i < totalItems; ++i)
        {
            const auto& d = descriptors[i];
            const auto pos = position_for_index(i);

            const int bw = static_cast<int>(std::round(d.size.x));
            const int bh = static_cast<int>(std::round(d.size.y));

            if (mx >= pos.first && mx <= pos.first + bw &&
                my >= pos.second && my <= pos.second + bh)
            {
                hover = i;
                break;
            }
        }

        const int cols = (std::max)(1, columns);
        const int rowsLocal = (std::max)(1, rows);

        if (leftPressed && !prevLeft)  selection = (selection == 0) ? static_cast<std::size_t>(totalItems - 1) : selection - 1;
        if (rightPressed && !prevRight) selection = (selection + 1) % static_cast<std::size_t>(totalItems);

        if (upPressed && !prevUp)
        {
            selection = (selection < static_cast<std::size_t>(cols))
                ? (selection + static_cast<std::size_t>((rowsLocal - 1) * cols))
                : (selection - static_cast<std::size_t>(cols));
            if (selection >= static_cast<std::size_t>(totalItems))
                selection = static_cast<std::size_t>(totalItems - 1);
        }

        if (downPressed && !prevDown)
        {
            const auto next = selection + static_cast<std::size_t>(cols);
            selection = (next >= static_cast<std::size_t>(totalItems))
                ? (selection % static_cast<std::size_t>(cols))
                : next;
        }

        if (!upPressed && !downPressed && !leftPressed && !rightPressed && hover >= 0)
            selection = static_cast<std::size_t>(hover);

        prevUp = upPressed; prevDown = downPressed;
        prevLeft = leftPressed; prevRight = rightPressed;

        const float windowPadding = LayoutSpacing * 0.5f;
        const gui::Vec2 windowPos{
            (std::max)(0.0f, layoutOriginX - windowPadding),
            (std::max)(0.0f, layoutOriginY - windowPadding)
        };
        const gui::Vec2 windowSize{
            layoutWidth + windowPadding * 2.0f,
            layoutHeight + windowPadding * 2.0f
        };

        gui::begin_window("Main Menu", windowPos, windowSize);

        std::optional<Choice> chosen{};
        for (int i = 0; i < totalItems; ++i)
        {
            const auto pos = position_for_index(i);
            gui::set_cursor({ static_cast<float>(pos.first), static_cast<float>(pos.second) });

            auto label = descriptors[i].label;
            if (static_cast<std::size_t>(i) == selection)
                label = "> " + label + " <";

            const bool activated = gui::button(label, descriptors[i].size);

            if (core::cli::trace_menu_button0_rect && i == 0)
            {
                if (auto bounds = gui::last_button_bounds())
                {
                    const std::array<float, 4> rect{
                        bounds->position.x, bounds->position.y,
                        bounds->size.x,     bounds->size.y
                    };

                    static std::array<float, 4> logged{};
                    static bool hasLogged = false;

                    auto approx_equal = [](float a, float b) { return std::fabs(a - b) <= 0.5f; };

                    bool changed = !hasLogged;
                    if (!changed)
                    {
                        for (std::size_t k = 0; k < rect.size(); ++k)
                        {
                            if (!approx_equal(rect[k], logged[k])) { changed = true; break; }
                        }
                    }

                    if (changed)
                    {
                        logged = rect;
                        hasLogged = true;

                        std::ostringstream oss;
                        oss.setf(std::ios::fixed, std::ios::floatfield);
                        oss << std::setprecision(2)
                            << "[MenuOverlay] button0 GUI bounds: x=" << rect[0]
                            << " y=" << rect[1]
                            << " w=" << rect[2]
                            << " h=" << rect[3];

                        if (flipVertical) oss << " (OpenGL input flip)";
                        oss << '\n';

                        std::cout << oss.str();
                    }
                }
            }

            if (activated)
            {
                selection = static_cast<std::size_t>(i);
                chosen = descriptors[i].choice;
            }
        }

        gui::end_window();

        const bool triggeredByEnter = enterPressed && !prevEnter;

        if (chosen.has_value())
        {
            prevEnter = enterPressed;
            return chosen;
        }

        if (triggeredByEnter)
        {
            prevEnter = enterPressed;
            return static_cast<Choice>(selection);
        }

        prevEnter = enterPressed;
        return std::nullopt;
    }

    void MenuOverlay::cleanup()
    {
        descriptors.clear();
        cachedPositions.clear();
        colWidths.clear();
        rowHeights.clear();

        cachedWidth = -1;
        cachedHeight = -1;
        columns = 1;
        rows = 0;

        layoutOriginX = 0.0f;
        layoutOriginY = 0.0f;
        layoutWidth = 0.0f;
        layoutHeight = 0.0f;

        selection = 0;
        prevUp = prevDown = prevLeft = prevRight = prevEnter = false;

        initialized = false;
    }
}
