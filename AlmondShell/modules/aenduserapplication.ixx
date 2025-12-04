module;

import <memory>;

import "aapplicationmodule.hpp";
import "acontext.hpp";
import "aeventsystem.hpp";
import "ainput.hpp";
import "arobusttime.hpp";

export module aenduserapplication;

export namespace almondnamespace::app
{
    // ─── Fixed‑step config ──────────────────────────────────────────────
    inline constexpr double STEP_S = 0.150;   // 150 ms per frame

    void translate_input(const core::Context& ctx) noexcept;

    // ─── Main loop (ECS-free stub – slots neatly into your engine) ─────
    bool run_app(std::shared_ptr<almondnamespace::core::Context>& ctx);
}
