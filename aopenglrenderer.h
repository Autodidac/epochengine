#pragma once

#include "aengineconfig.h"

#ifdef ALMOND_USING_OPENGL

//#include "alsOpenGLFreeType.h"
//#include "alsOpenGLRenderMode.h"
//#include "alsOpenGLTexture.h"
//#include "aopenglshader.h"
//#include "alsOpenGLTextureAtlas.h"
//#include "alsTextureAtlasPacker.h"

#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <cassert>
#include <iostream>

namespace almondnamespace::opengl {

    struct OpenGLContext {};

    inline void drawSprite(uint32_t atlasId, float x, float y, float width, float height) {
        std::cout << "Drawing sprite with ID " << atlasId << " at (" << x << ", " << y << ") with size (" << width << ", " << height << ") from OpenGL context\n";

        // opengl drawing logic goes here.
    }


    // Forward declaration of Renderer class
    //class Renderer;

   // class Renderer {
  //  public:
/*        Renderer(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
            : m_renderMode(RenderMode::SingleTexture) {
            m_shader = std::make_shared<ShaderProgram>(vertexShaderPath, fragmentShaderPath);
        }

        Renderer(OpenGLTextureAtlas atlas, const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
            : m_textureatlas(atlas), m_renderMode(RenderMode::TextureAtlas) {
            m_shader = std::make_shared<ShaderProgram>(vertexShaderPath, fragmentShaderPath);
        }
*/

     //   Renderer();
     //   ~Renderer() = default;

        //std::shared_ptr<ShaderProgram> GetShader() const {
        //    return m_shader;
        //}

        //void SetRenderMode(RenderMode mode) {
        //    m_renderMode = mode;
        //}

        //bool GetRenderMode(RenderMode mode) const {
        //    return m_renderMode == mode;
        //}
        ///*         // need to fix this
        //        void SetTextureAtlas(OpenGLTextureAtlas& atlas) {
        //            textureAtlas = &atlas;
        //            renderMode = RenderMode::TextureAtlas;
        //        }
        //*/
        //void AddTextureToAtlases(std::vector<OpenGLTextureAtlas>& atlases, const std::filesystem::path& texturePath) {
        //    for (auto& atlas : atlases) {
        //        try {
        //            atlas.AddTextureToAtlas(texturePath);
        //            return; // Successfully added
        //        }
        //        catch (const std::exception& e) {
        //            // If atlas is full, continue to the next atlas
        //            if (std::string(e.what()).find("Texture is too large for the current atlas size.") == std::string::npos) {
        //                throw; // Other errors should propagate
        //            }
        //        }
        //    }

        //    // Create a new atlas if none of the existing ones can accommodate the texture
        //    OpenGLTextureAtlas newAtlas(texturePath, almond::Texture::Format::RGBA8);
        //    newAtlas.AddTextureToAtlas(texturePath);
        //    atlases.push_back(std::move(newAtlas));
        //}
        ///*
        //        void RepackAtlas(int atlasWidth, int atlasHeight) {
        //            // Ensure there is at least one atlas
        //            if (m_atlasTextures.empty()) {
        //                m_atlasTextures.emplace_back(atlasWidth, atlasHeight);
        //            }

        //            for (auto& atlas : m_atlasTextures) {
        //                TexturePacker packer(atlasWidth, atlasHeight);

        //                for (auto& texture : atlas.GetTextures()) {
        //                    auto [x, y] = packer.Insert(texture.GetWidth(), texture.GetHeight());
        //                    if (x == -1 || y == -1) {
        //                        throw std::runtime_error("Failed to repack texture: insufficient space in atlas.");
        //                    }
        //                    texture.SetAtlasPosition(x, y);
        //                }
        //            }
        //        }
        //*/

        //OpenGLTexture& GetTexture(const std::string& texturePath) {
        //    for (auto& tex : m_textures) {
        //        if (tex.GetPath() == texturePath) {
        //            return tex;
        //        }
        //    }
        //    throw std::runtime_error("Texture not found in the renderer.");
        //}

        //OpenGLTextureAtlas& GetTextureAtlas(const std::filesystem::path& texturePath) {
        //    // Iterate through all atlas textures to find the atlas containing the requested texture path
        //    for (auto& texAtlas : m_textureatlases) {
        //        if (texAtlas.GetPath() == texturePath) {
        //            return texAtlas;
        //        }
        //    }
        //    throw std::runtime_error("Texture atlas not found for the given path: " + texturePath.string());
        //}

        //OpenGLTexture GetAtlasTexture(const std::filesystem::path& texturePath) {
        //    if (!&m_textureatlas) {
        //        throw std::runtime_error("Texture atlas is not initialized.");
        //    }

        //    if (texturePath.empty()) {
        //        throw std::runtime_error("Texture path is empty.");
        //    }

        //    try {
        //        // Assuming m_textureatlases is a vector, access the relevant atlas
        //        auto myatlastexture = m_textureatlases.at(0).GetTexture(texturePath);
        //        // m_texture = myatlastexture;

        //       // Retrieve texture information from the selected atlas
        //        auto [xOffset, yOffset, width, height] = m_textureatlases.at(0).GetAtlasTextureMap(texturePath.string());
        //        std::cout << "Texture Offset: (" << xOffset << ", " << yOffset
        //            << "), Size: (" << width << "x" << height << ")\n";

        //        return m_texture;
        //    }
        //    catch (const std::exception& e) {
        //        // Handle any potential errors (like missing texture or incorrect atlas lookup)
        //        std::cerr << "Error retrieving texture: " << e.what() << '\n';
        //        //  return; // or handle it according to your needs (e.g., return a default texture)
        //        throw; // Re-throw the exception for further handling
        //    }
        //}

        //void AddTexture(const std::filesystem::path& filepath, int xOffset, int yOffset) {
        //    if (filepath.empty()) {
        //        std::cerr << "Texture path is empty. Using default texture." << std::endl;
        //        m_textures.emplace_back("../../images/bmp3.bmp", almond::Texture::Format::RGBA8, false);
        //        return;
        //    }

        //    if (m_renderMode == RenderMode::TextureAtlas) {
        //        bool textureAdded = false;
        //        int texWidth = 4096;
        //        int texHeight = 4096;

        //        // Try to add the texture to an existing atlas
        //        for (auto& atlas : m_atlastextures) {
        //            auto [Offsetx, Offsety, texWidth, texHeight] = atlas.TryAddTexture(filepath);
        //            if (Offsetx != -1 && Offsety != -1) {
        //                textureAdded = true;
        //                // Update the atlas with the new texture data
        //                atlas.UpdateAtlasData(filepath, Offsetx, Offsety, texWidth, texHeight);
        //                break;
        //            }
        //        }

        //        // If no suitable atlas was found, create a new one
        //        if (!textureAdded) {
        //            std::cout << "Failed to add texture '" + filepath.string() + "' to atlas. \nCreating new atlas of size: x " << texWidth << " y " << texHeight << "\n";
        //            m_atlastextures.emplace_back(filepath, Texture::Format::RGBA8, false, texWidth, texHeight); // Create new atlas in renderer
        //            auto& newAtlas = m_atlastextures.back();
        //            auto [newOffsetx, newOffsety, newTexWidth, newTexHeight] = newAtlas.TryAddTexture(filepath);

        //            if (newOffsetx == -1 || newOffsety == -1) {
        //                throw std::runtime_error("Failed to add texture '" + filepath.string() + "' to a new atlas.");
        //            }

        //            // Update the atlas with the new texture data
        //            newAtlas.UpdateAtlasData(filepath, newOffsetx, newOffsety, newTexWidth, newTexHeight);
        //        }
        //    }
        //    else {
        //        // Non-atlas mode: add texture directly to the list
        //        m_textures.emplace_back(filepath, almond::Texture::Format::RGBA8, false);
        //    }
        //}

        //void AddMesh(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
        //    if (m_meshes.find(name) == m_meshes.end()) {
        //        m_meshes.emplace(name, std::make_shared<almond::Mesh>(vertices, indices));
        //    }
        //    else {
        //        std::cerr << "Mesh with the name: " << name << " already exists!" << std::endl;
        //    }
        //}

        //void AddQuad(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
        //    if (m_quads.find(name) == m_quads.end()) {
        //        m_quads.emplace(name, std::make_shared<almond::Quad>(vertices, indices));
        //    }
        //    else {
        //        std::cerr << "Quad with the name: " << name << " already exists!" << std::endl;
        //    }
        //}

        //void SetCommonUniforms(ShaderProgram* shader, int textureSlot) {
        //    shader->SetUniform("textureSampler", textureSlot);
        //    shader->SetUniform("scale", glm::vec2(1.0f, 1.0f));
        //}

        //std::shared_ptr<almond::Mesh> GetMesh(const std::string& name) {
        //    auto it = m_meshes.find(name);
        //    return (it != m_meshes.end()) ? it->second : nullptr;
        //}

        //std::shared_ptr<almond::Quad> GetQuad(const std::string& name) {
        //    auto it = m_quads.find(name);
        //    return (it != m_quads.end()) ? it->second : nullptr;
        //}

        //// The Draw Functions
        //void DrawSingleQuad(const std::shared_ptr<Quad>& quad, const OpenGLTexture& texture, float x, float y) {
        //    m_shader->Use();

        //    SetCommonUniforms(m_shader.get(), 0);

        //    if (m_renderMode == RenderMode::TextureAtlas) {
        //        //                if (textureAtlas) textureAtlas->Bind();
        //        if (!m_atlastextures.empty()) {
        //            m_atlastextures[0].Bind(0);
        //        }
        //    }
        //    else {
        //        texture.Bind(0);
        //    }

        //    SetupVAO(quad);

        //    glBindVertexArray(quad->GetVAO());
        //    glDrawElements(GL_TRIANGLES, quad->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        //    glBindVertexArray(0);

        //    if (m_renderMode == RenderMode::TextureAtlas) {
        //        //if (textureAtlas) textureAtlas->Unbind();
        //        if (!m_atlastextures.empty()) {
        //            m_atlastextures[0].Unbind();
        //        }
        //    }
        //    else {
        //        texture.Unbind();
        //    }
        //}

        //void DrawBatch(const std::shared_ptr<Quad>& quad, OpenGLTextureAtlas atlasTexture, const std::vector<std::shared_ptr<almond::RenderCommand>>& commands) {
        //    if (commands.empty()) return;

        //    m_shader->Use();
        //    SetCommonUniforms(m_shader.get(), 0);

        //    if (m_renderMode == RenderMode::TextureAtlas) {
        //        if (!m_atlastextures.empty()) {
        //            m_atlastextures[0].Bind(0);
        //        }
        //    }
        //    else {
        //        if (!m_textures.empty()) {
        //            m_textures[0].Bind(0);
        //        }
        //        else {
        //            std::cerr << "No single texture loaded to render!" << std::endl;
        //            return;
        //        }
        //    }

        //    // Execute uniform commands
        //    for (const auto& cmd : commands) {
        //        if (cmd->GetType() == RenderCommand::CommandType::SetUniform) {
        //            cmd->Execute(m_shader.get());
        //        }
        //    }

        //    // Execute draw commands
        //    for (const auto& cmd : commands) {
        //        if (auto drawCmd = dynamic_cast<const DrawCommand*>(cmd.get())) {
        //            // experimental
        //            SetupVAO(quad);

        //            glBindVertexArray(quad->GetVAO());
        //            //            glDrawElements(GL_TRIANGLES, quad->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        //            //            glBindVertexArray(0);
        //                        //
        //                              //  SetupVAO(drawCmd->GetQuad());
        //                            //    glBindVertexArray(drawCmd->GetQuad()->GetVAO());
        //            drawCmd->Execute(m_shader.get());
        //            glBindVertexArray(0);
        //        }
        //    }

        //    if (m_renderMode == RenderMode::TextureAtlas) {
        //        //                if (textureAtlas) textureAtlas->Unbind();
        //        if (!m_atlastextures.empty()) {
        //            m_atlastextures[0].Bind(0);
        //        }
        //    }
        //    else {
        //        if (!m_textures.empty()) {
        //            m_textures[0].Unbind();
        //        }
        //    }
        //}
        /*
                void RenderSingleGlyph(FontManager& fontManager, char character, float x, float y, float scale, const glm::vec3& color) {
                    const FontManager::Character& glyphQuad = fontManager.getCharacter(character);

                    // Activate shader and set uniforms
                    shader->Use();
                    shader->SetUniform("textColor", color);
                    SetCommonUniforms(shader.get(), 0);

                    // Calculate the position and size of the glyph
                    float xpos = x + glyphQuad.bearing.x * scale;
                    float ypos = y - (glyphQuad.size.y - glyphQuad.bearing.y) * scale;
                    float w = glyphQuad.size.x * scale;
                    float h = glyphQuad.size.y * scale;

                    // Update VBO for glyph quad geometry
                    float vertices[6][4] = {
                        { xpos,     ypos + h,   0.0f, 0.0f },
                        { xpos,     ypos,       0.0f, 1.0f },
                        { xpos + w, ypos,       1.0f, 1.0f },
                        { xpos,     ypos + h,   0.0f, 0.0f },
                        { xpos + w, ypos,       1.0f, 1.0f },
                        { xpos + w, ypos + h,   1.0f, 0.0f }
                    };

                    // Bind glyph texture
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, glyphQuad.GetTextureID());

                    // Bind VAO and update VBO with new vertices
                    glBindVertexArray(glyphQuad.GetVAO());
                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

                    // Enable vertex attributes (position and texture coordinates)
                    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
                    glEnableVertexAttribArray(1);

                    // Draw the glyph quad
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    // Clean up
                    glDeleteBuffers(1, &vbo);
                    glBindVertexArray(0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
        */
        //void InitializeFontRenderingResources(GLuint& outVAO, GLuint& outVBO, GLuint& outEBO) {
        //    glGenVertexArrays(1, &outVAO);
        //    glGenBuffers(1, &outVBO);
        //    glGenBuffers(1, &outEBO);

        //    glBindVertexArray(outVAO);

        //    // Setup VBO
        //    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
        //    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_STATIC_DRAW);

        //    glEnableVertexAttribArray(0);
        //    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        //    glEnableVertexAttribArray(1);
        //    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        //    // Setup EBO
        //    unsigned int indices[] = {
        //        0, 1, 2,  // First triangle
        //        0, 2, 3   // Second triangle
        //    };
        //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outEBO);
        //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        //    // Unbind buffers
        //    glBindBuffer(GL_ARRAY_BUFFER, 0);
        //    glBindVertexArray(0);
        //}


        //void SetupVAO(const std::shared_ptr<Quad>& quad) {
        //    if (!quad->GetVAO()) {

        //        unsigned int VAO;
        //        glGenVertexArrays(1, &VAO);
        //        glBindVertexArray(VAO);

        //        // Bind the VBO (only if not already bound)
        //        GLuint VBO = quad->GetVBO();
        //        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        //        // Bind the EBO (only if not already bound)
        //        GLuint EBO = quad->GetEBO();
        //        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        //        // Vertex position attribute (location 0)
        //        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        //        glEnableVertexAttribArray(0);

        //        // Vertex texture coordinates attribute (location 1)
        //        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        //        glEnableVertexAttribArray(1);

        //        // Unbind the VAO to ensure subsequent calls don't affect it
        //        glBindVertexArray(0);

        //        // Store the VAO in the quad object for reuse
        //        quad->SetVAO(VAO);

        //    }
   //    };
   // private:
      //  GLint m_maxTextureUnits = GL_MAX_TEXTURE_IMAGE_UNITS;
     //   GLint m_maxTextureSize = GL_MAX_TEXTURE_SIZE;
    //    GLint m_max3dTextureSize = GL_MAX_3D_TEXTURE_SIZE;

        /*
                GLint maxTextureSize;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
                std::cout << "Maximum 1D/2D texture size: y " << maxTextureSize << " x " << maxTextureSize << std::endl;

                GLint max3dTextureSize;
                glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3dTextureSize);
                std::cout << "Maximum 3D texture size: y " << max3dTextureSize << " x " << max3dTextureSize << std::endl;

                GLint maxTextureUnits;
                glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
                std::cout << "Max texture units: " << maxTextureUnits << std::endl;
        */
        //std::shared_ptr<ShaderProgram> m_shader;
        //OpenGLTexture m_texture;
        //OpenGLTextureAtlas m_textureatlas;
        //std::vector<OpenGLTexture> m_textures;
        //std::vector<OpenGLTexture> m_atlastextures;
        //std::vector<OpenGLTextureAtlas> m_textureatlases;
        //std::unordered_map<std::string, std::tuple<int, int, int, int>> m_textureMap;
        //std::unordered_map<std::string, std::shared_ptr<Quad>> m_quads;
        //std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
        //RenderMode m_renderMode;
   // };

    //class FontRenderer {
    //public:
    //    FontRenderer(FontManager& fontManager, Renderer& renderer, GLuint vao, GLuint vbo, GLuint ebo)
    //        : fontManager(fontManager), renderer(renderer), VAO(vao), VBO(vbo), EBO(ebo) {
    //    }

    //    void RenderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    //        // Activate the shader
    //        renderer.GetShader()->Use();
    //        renderer.GetShader()->SetUniform("textColor", color);

    //        // Bind the VAO
    //        glBindVertexArray(VAO);

    //        for (char c : text) {
    //            const FontManager::Character& ch = fontManager.getCharacter(c);

    //            float xpos = x + ch.bearing.x * scale;
    //            float ypos = y - (ch.size.y - ch.bearing.y) * scale;
    //            float w = ch.size.x * scale;
    //            float h = ch.size.y * scale;

    //            // Update VBO with quad vertices
    //            float vertices[6][4] = {
    //                { xpos, ypos + h, 0.0f, 0.0f },
    //                { xpos, ypos, 0.0f, 1.0f },
    //                { xpos + w, ypos, 1.0f, 1.0f },
    //                { xpos, ypos + h, 0.0f, 0.0f },
    //                { xpos + w, ypos, 1.0f, 1.0f },
    //                { xpos + w, ypos + h, 1.0f, 0.0f }
    //            };

    //            glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    //            // Bind EBO
    //            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    //            // Render glyph texture over the quad
    //            glBindTexture(GL_TEXTURE_2D, ch.textureID);

    //            // Draw the quad
    //            //glDrawArrays(GL_TRIANGLES, 0, 6);
    //            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    //            // Advance cursor for the next glyph
    //            x += (ch.advance >> 6) * scale; // Bitshift by 6 to get pixel value
    //        }

    //        // Unbind the VAO and texture
    //        glBindVertexArray(0);
    //        glBindTexture(GL_TEXTURE_2D, 0);
    //    }

    //private:
    //    FontManager& fontManager;
    //    Renderer& renderer;
    //    GLuint VAO;
    //    GLuint VBO;
    //    GLuint EBO;
    //};


}

#endif