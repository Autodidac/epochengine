#pragma once

#include "aplatform.h"
#include "aspritebank.h"
#include "atextureatlas.h"

#ifdef ALMOND_USING_VULKAN
#include "avulkanrenderer.h"
#endif
#ifdef ALMOND_USING_OPENGL
#include "aopenglrenderer.h"
#endif

#include <iostream>
#include <memory>
#include <stdexcept>
#include <optional>

namespace almondnamespace {

    struct CommandBuffer {
        std::vector<std::string> commands;

        void add_command(const std::string& command) {
            commands.push_back(command);
        }

        // Placeholder to represent actual Vulkan/OpenGL, etc commands.
        void execute() const {
            for (const auto& cmd : commands) {
                std::cout << "Executing: " << cmd << std::endl;
            }
        }
    };

    struct Renderer {
        std::shared_ptr<TextureAtlas> textureAtlas;
        std::shared_ptr<SpriteBank> spriteBank;

        // Context for Vulkan or OpenGL
        void* platformContext;

        // Constructor that initializes the Renderer with TextureAtlas, SpriteBank, and platform context
        Renderer(std::shared_ptr<TextureAtlas> atlas, std::shared_ptr<SpriteBank> bank, void* context)
            : textureAtlas(std::move(atlas)), spriteBank(std::move(bank)), platformContext(context) {
            if (!textureAtlas || !spriteBank) {
                throw std::invalid_argument("TextureAtlas or SpriteBank cannot be null during initialization.");
            }
        }

        bool is_initialized() const {
            return textureAtlas && spriteBank;
        }

        bool set_shader_pipeline(const std::string& vertPath, const std::string& fragPath) const {

            platform_set_shader_pipeline(vertPath, fragPath);

            std::cerr << "set_shader_pipeline is not implemented for this renderer." << std::endl;
            return false;
        }

        void draw_sprite(const std::string& spriteName, CommandBuffer& commandBuffer) const {
            if (!is_initialized()) {
                std::cerr << "Renderer is not initialized (TextureAtlas or SpriteBank is null)." << std::endl;
                return;
            }

            auto sprite = spriteBank->get_sprite(spriteName);
            if (!sprite) {
                std::cerr << "Sprite '" << spriteName << "' not found in SpriteBank." << std::endl;
                return;
            }

            // Using structured binding (if sprite exists) to get the sprite's data
            auto [atlasId, x, y, width, height] = *sprite;

            // Add the draw command to the buffer
            commandBuffer.add_command("Draw sprite: " + spriteName);
            platform_draw_sprite(atlasId, x, y, width, height);
        }

    private:
        // Platform-specific draw sprite method (this would be Vulkan or OpenGL)
        void platform_draw_sprite(int atlasId, float x, float y, float width, float height) const {
#ifdef ALMOND_USING_VULKAN

           
            if (platformContext) {
                // Pass the context to Vulkan's draw function if Vulkan is being used
                vulkan::drawSprite(atlasId, x, y, width, height);
            }


#elif defined(ALMOND_USING_OPENGL)
            if (platformContext) {
                // Pass the context to OpenGL's draw function if OpenGL is being used
                opengl::drawSprite(atlasId, x, y, width, height);
            }
#else
            std::cerr << "No renderer selected (Vulkan or OpenGL required)." << std::endl;
            return;
#endif
        }

        void platform_set_shader_pipeline(const std::string& vertPath, const std::string& fragPath) const {
#ifdef ALMOND_USING_VULKAN
            if (platformContext) {
                // Pass the context to Vulkan's draw function if Vulkan is being used
                vulkan::set_shader_pipeline(vertPath, fragPath);
            }


#elif defined(ALMOND_USING_OPENGL)

            if (platformContext) {
                // Pass the context to Vulkan's draw function if Vulkan is being used
                opengl::set_shader_pipeline(vertPath, fragPath);
            }
#else
            std::cerr << "No renderer selected (Vulkan or OpenGL required)." << std::endl;
            return;
#endif
        }

    };

} // namespace almondnamespace
