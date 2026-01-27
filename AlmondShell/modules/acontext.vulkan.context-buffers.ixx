// ============================================================================
// modules/acontext.vulkan.context-buffers.ixx
// Export surface for buffer-related Application methods (declarations only).
// ============================================================================

module;

// export the partitions in this module unit that need to be visible to client-side users

export module acontext.vulkan.context:buffers;

import :shared_context; // ensures Application is visible to importers
import :meshcube;       // if callers need cube_* declarations
