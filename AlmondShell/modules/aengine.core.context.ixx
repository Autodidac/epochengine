module;

#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
#   include <windows.h> // HWND/HDC/HGLRC
#endif

export module aengine.core.context;

// Standard library (header units or std module; keep consistent project-wide)
import <algorithm>;
import <cstdint>;
import <functional>;
import <map>;
import <memory>;
import <mutex>;
import <queue>;
import <shared_mutex>;
import <span>;
import <string>;
import <utility>;
import <vector>;

// Project modules (these must be modules too; do NOT include headers)
import aengine.context.type;         // ContextType
import aengine.context.commandqueue; // CommandQueue
import aatomicfunction;              // AlmondAtomicFunction
import aengine.context.window;       // core::WindowData
import aengine.input;                // input::Key, input::MouseButton, mouseX/mouseY + helpers
import aatlas.texture;               // TextureAtlas
import aspritehandle;                // SpriteHandle
import aimage.loader;                // ImageData

export namespace almondnamespace::core
{
    class MultiContextManager;

    export class Context
    {
        friend class MultiContextManager;
        friend class aengine;

    public:
        // --- Backend render hooks (fast raw function pointers) ---
        using InitializeFunc = void(*)();
        using CleanupFunc = void(*)();
        using ProcessFunc = bool(*)(std::shared_ptr<Context>, core::CommandQueue&);
        using ClearFunc = void(*)();
        using PresentFunc = void(*)();
        using GetWidthFunc = int(*)();
        using GetHeightFunc = int(*)();
        using RegistryGetFunc = int(*)(const char*);

        using DrawSpriteFunc = void(*)(SpriteHandle,
            std::span<const TextureAtlas* const>,
            float, float, float, float);

        using AddTextureFunc = std::uint32_t(*)(TextureAtlas&, std::string, const ImageData&);
        using AddAtlasFunc = std::uint32_t(*)(const TextureAtlas&);
        using AddModelFunc = int(*)(const char*, const char*);

        Context() = default;

        Context(std::string name,
            core::ContextType t,
            InitializeFunc init,
            CleanupFunc cln,
            ProcessFunc proc,
            ClearFunc clr,
            PresentFunc pres,
            GetWidthFunc gw,
            GetHeightFunc gh,
            RegistryGetFunc rg,
            DrawSpriteFunc ds,
            AddTextureFunc at,
            AddAtlasFunc aa,
            AddModelFunc am)
            : type(t),
            backendName(std::move(name)),
            initialize(init),
            cleanup(cln),
            process(proc),
            clear(clr),
            present(pres),
            get_width(gw),
            get_height(gh),
            registry_get(rg),
            draw_sprite(ds),
            add_model(am)
        {
            add_texture.store(at);
            add_atlas.store(aa);
        }

        // --- Safe wrappers ---
        void initialize_safe() const noexcept { if (initialize) initialize(); }
        void cleanup_safe()    const noexcept { if (cleanup) cleanup(); }

        // implemented out-of-line (keeps module interface lean)
        bool process_safe(std::shared_ptr<Context> ctx, core::CommandQueue& queue);

        void clear_safe()   const noexcept { if (clear) clear(); }

        // Back-compat: older call sites used ctx->clear_safe(ctx)
        void clear_safe(std::shared_ptr<Context>) const noexcept { clear_safe(); }

        void present_safe() const noexcept { if (present) present(); }

        int get_width_safe()  const noexcept { return get_width ? get_width() : width; }
        int get_height_safe() const noexcept { return get_height ? get_height() : height; }

        bool is_key_held_safe(input::Key k) const noexcept
        {
            return is_key_held ? is_key_held(k) : false;
        }

        bool is_key_down_safe(input::Key k) const noexcept
        {
            return is_key_down ? is_key_down(k) : false;
        }

        void get_mouse_position_safe(int& x, int& y) const noexcept
        {
            if (get_mouse_position)
            {
                get_mouse_position(x, y);
                return;
            }

            x = input::mouseX.load(std::memory_order_acquire);
            y = input::mouseY.load(std::memory_order_acquire);
        }

        bool is_mouse_button_held_safe(input::MouseButton b) const noexcept
        {
            return is_mouse_button_held ? is_mouse_button_held(b)
                : input::is_mouse_button_held(b);
        }

        bool is_mouse_button_down_safe(input::MouseButton b) const noexcept
        {
            return is_mouse_button_down ? is_mouse_button_down(b)
                : input::is_mouse_button_down(b);
        }

        int registry_get_safe(const char* key) const noexcept
        {
            return registry_get ? registry_get(key) : 0;
        }

        void draw_sprite_safe(SpriteHandle h,
            std::span<const TextureAtlas* const> atlases,
            float x, float y, float w, float hgt) const noexcept
        {
            if (draw_sprite) draw_sprite(h, atlases, x, y, w, hgt);
        }

        std::uint32_t add_texture_safe(TextureAtlas& atlas,
            std::string name,
            const ImageData& img) const noexcept
        {
            try { return add_texture(atlas, std::move(name), img); }
            catch (...) { return 0u; }
        }

        std::uint32_t add_atlas_safe(const TextureAtlas& atlas) const noexcept
        {
            try { return add_atlas(atlas); }
            catch (...) { return 0u; }
        }

        int add_model_safe(const char* name, const char* path) const noexcept
        {
            return add_model ? add_model(name, path) : -1;
        }

        // -----------------------------------------------------------------
        // Legacy public pointer:
        // Many game/sample modules expect to read ctx->windowData.
        // Keeping it public avoids churn while the engine is mid-migration.
        // -----------------------------------------------------------------
        WindowData* windowData = nullptr;

    private:
        void* native_window = nullptr;
        void* native_drawable = nullptr;
        void* native_gl_context = nullptr;

#if defined(_WIN32) && !defined(ALMOND_MAIN_HEADLESS)
        HWND  hwnd = nullptr;
        HDC   hdc = nullptr;
        HGLRC hglrc = nullptr;
#endif

        // logical canvas
        int width = 400;
        int height = 300;

        // physical framebuffer size
        int framebufferWidth = 400;
        int framebufferHeight = 300;

        // virtual design canvas
        int virtualWidth = 400;
        int virtualHeight = 300;

        ContextType type = ContextType::Custom;
        std::string backendName{};

        // --- Backend function pointers ---
        InitializeFunc  initialize = nullptr;
        CleanupFunc     cleanup = nullptr;
        ProcessFunc     process = nullptr;
        ClearFunc       clear = nullptr;
        PresentFunc     present = nullptr;
        GetWidthFunc    get_width = nullptr;
        GetHeightFunc   get_height = nullptr;
        RegistryGetFunc registry_get = nullptr;
        DrawSpriteFunc  draw_sprite = nullptr;
        AddModelFunc    add_model = nullptr;

        // --- Input hooks (std::function for flexibility) ---
        std::function<bool(input::Key)>         is_key_held;
        std::function<bool(input::Key)>         is_key_down;
        std::function<void(int&, int&)>         get_mouse_position; // must be client coords
        std::function<bool(input::MouseButton)> is_mouse_button_held;
        std::function<bool(input::MouseButton)> is_mouse_button_down;

        // --- High-level callbacks ---
        core::AlmondAtomicFunction<std::uint32_t(TextureAtlas&, std::string, const ImageData&)> add_texture;
        core::AlmondAtomicFunction<std::uint32_t(const TextureAtlas&)>                         add_atlas;
        std::function<void(int, int)>                                                          onResize;
    };

    export struct BackendState
    {
        std::shared_ptr<Context>              master;
        std::vector<std::shared_ptr<Context>> duplicates;
        std::unique_ptr<void, void(*)(void*)> data{ nullptr, [](void*) {} };
    };

    export using BackendMap = std::map<core::ContextType, BackendState>;

    export extern BackendMap        g_backends;
    export extern std::shared_mutex g_backendsMutex;

    export void InitializeAllContexts();
    export std::shared_ptr<Context> CloneContext(const Context& prototype);
    export void AddContextForBackend(core::ContextType type, std::shared_ptr<Context> context);
    export bool ProcessAllContexts();
}

export namespace almondnamespace::context
{
    export using Context = almondnamespace::core::Context;
    export using BackendState = almondnamespace::core::BackendState;
    export using BackendMap = almondnamespace::core::BackendMap;

    export using almondnamespace::core::g_backends;
    export using almondnamespace::core::g_backendsMutex;

    export using almondnamespace::core::InitializeAllContexts;
    export using almondnamespace::core::CloneContext;
    export using almondnamespace::core::AddContextForBackend;
    export using almondnamespace::core::ProcessAllContexts;
}
