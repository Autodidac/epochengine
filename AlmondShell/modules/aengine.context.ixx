export module aengine.context;

// Export the existing context declarations for both modular and
// non-modular translation units. The legacy header remains the
// single source of truth for the public API while this module
// re-exports it for BMI consumers.
export import "acontext.hpp";
