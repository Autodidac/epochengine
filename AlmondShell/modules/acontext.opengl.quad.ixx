/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
 // acontext.opengl.quad.ixx  (was aopenglquad.hpp)

module;

// Global module fragment: platform macros + GL loader.
#include "..\include\aengine.config.hpp"  // brings in <glad/glad.h> when ALMOND_USING_OPENGL

#if defined(ALMOND_USING_OPENGL)
#   include <glad/glad.h>
#endif

export module acontext.opengl.quad;

import acontext.opengl.state;
import acontext.opengl.platform;

import <iostream>;
import <utility>;

#if defined(ALMOND_USING_OPENGL)

export namespace almondnamespace::openglquad
{
    class Quad
    {
        GLuint vao_ = 0;
        GLuint vbo_ = 0;
        GLuint ebo_ = 0;

        // Interleaved: pos.xy, uv.xy
        static constexpr float defaultVertices[16] = {
            // x,   y,   u,   v
             0.f, 0.f, 0.f, 0.f,
             1.f, 0.f, 1.f, 0.f,
             1.f, 1.f, 1.f, 1.f,
             0.f, 1.f, 0.f, 1.f
        };

        static constexpr unsigned int defaultIndices[6] = {
            0, 1, 2,
            2, 3, 0
        };

    public:
        Quad()
        {
            // VAO
            glGenVertexArrays(1, &vao_);
            glBindVertexArray(vao_);

            // VBO
            glGenBuffers(1, &vbo_);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(
                GL_ARRAY_BUFFER,
                sizeof(defaultVertices),
                defaultVertices,
                GL_STATIC_DRAW
            );

            // Attributes
            glEnableVertexAttribArray(0); // position
            glVertexAttribPointer(
                0, 2, GL_FLOAT, GL_FALSE,
                4 * sizeof(float),
                reinterpret_cast<void*>(0)
            );

            glEnableVertexAttribArray(1); // uv
            glVertexAttribPointer(
                1, 2, GL_FLOAT, GL_FALSE,
                4 * sizeof(float),
                reinterpret_cast<void*>(2 * sizeof(float))
            );

            // EBO
            glGenBuffers(1, &ebo_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sizeof(defaultIndices),
                defaultIndices,
                GL_STATIC_DRAW
            );

            // Unbind VAO (EBO stays associated with VAO)
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        ~Quad()
        {
            if (ebo_) glDeleteBuffers(1, &ebo_);
            if (vbo_) glDeleteBuffers(1, &vbo_);
            if (vao_) glDeleteVertexArrays(1, &vao_);
        }

        // Non-copyable
        Quad(const Quad&) = delete;
        Quad& operator=(const Quad&) = delete;

        // Movable
        Quad(Quad&& other) noexcept
            : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_)
        {
            other.vao_ = 0;
            other.vbo_ = 0;
            other.ebo_ = 0;
        }

        Quad& operator=(Quad&& other) noexcept
        {
            if (this != &other)
            {
                if (ebo_) glDeleteBuffers(1, &ebo_);
                if (vbo_) glDeleteBuffers(1, &vbo_);
                if (vao_) glDeleteVertexArrays(1, &vao_);

                vao_ = other.vao_;
                vbo_ = other.vbo_;
                ebo_ = other.ebo_;

                other.vao_ = 0;
                other.vbo_ = 0;
                other.ebo_ = 0;
            }
            return *this;
        }

        [[nodiscard]] GLuint  vao() const noexcept { return vao_; }
        [[nodiscard]] GLsizei index_count() const noexcept { return 6; }
    };

    inline void* LoadGLFunc(const char* name)
    {
        return almondnamespace::openglcontext::PlatformGL::get_proc_address(name);
    }

    inline GLuint compileShader(GLenum type, const char* src)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char log[512]{};
            glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
            throw std::runtime_error(std::format("Shader compile error: {}\nSource:\n{}", log, src));
        }
        return shader;
    }

    inline GLuint linkProgram(const char* vsSrc, const char* fsSrc)
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSrc);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[512]{};
            glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
            throw std::runtime_error(std::format("Program link error: {}", log));
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }

    inline const char* select_glsl_version(GLint major, GLint minor)
    {
        struct V { GLint major; GLint minor; const char* sv; };
        static constexpr V versions[] = {
            {4, 6, "#version 460 core"},
            {4, 5, "#version 450 core"},
            {4, 4, "#version 440 core"},
            {4, 3, "#version 430 core"},
            {4, 2, "#version 420 core"},
            {4, 1, "#version 410 core"},
            {4, 0, "#version 400 core"},
            {3, 3, "#version 330 core"},
        };

        for (const auto& c : versions)
            if (major > c.major || (major == c.major && minor >= c.minor))
                return c.sv;

        return "#version 330 core";
    }

    inline std::pair<GLint, GLint> parse_gl_version_string(const char* versionStr) noexcept
    {
        if (!versionStr) return { 0, 0 };

        std::string_view sv{ versionStr };
        auto p = sv.find_first_of("0123456789");
        if (p == std::string_view::npos) return { 0, 0 };
        sv.remove_prefix(p);

        auto dot = sv.find('.');
        if (dot == std::string_view::npos) return { 0, 0 };

        auto major_part = sv.substr(0, dot);
        sv.remove_prefix(dot + 1);

        auto minor_end = sv.find_first_not_of("0123456789");
        auto minor_part = sv.substr(0, minor_end);

        auto parse = [](std::string_view s) noexcept -> GLint {
            GLint v = 0;
            for (unsigned char ch : s)
            {
                if (ch < '0' || ch > '9') return 0;
                v = static_cast<GLint>(v * 10 + (ch - '0'));
            }
            return v;
            };

        return { parse(major_part), parse(minor_part) };
    }

    inline void destroy_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState) noexcept
    {
        if (glState.shader && glIsProgram(glState.shader))
            glDeleteProgram(glState.shader);

        glState.shader = 0;
        glState.uUVRegionLoc = -1;
        glState.uTransformLoc = -1;
        glState.uSamplerLoc = -1;

        if (glState.vao && glIsVertexArray(glState.vao))
            glDeleteVertexArrays(1, &glState.vao);
        if (glState.vbo && glIsBuffer(glState.vbo))
            glDeleteBuffers(1, &glState.vbo);
        if (glState.ebo && glIsBuffer(glState.ebo))
            glDeleteBuffers(1, &glState.ebo);

        glState.vao = 0;
        glState.vbo = 0;
        glState.ebo = 0;
    }

    inline bool build_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState)
    {
        destroy_quad_pipeline(glState);

        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        if (major == 0 && minor == 0)
        {
            const auto* vs = reinterpret_cast<const char*>(glGetString(GL_VERSION));
            auto [pm, pn] = parse_gl_version_string(vs);
            major = pm; minor = pn;
        }
        if (major == 0 && minor == 0) { major = 3; minor = 3; }

        const char* directive = select_glsl_version(major, minor);

        std::string vs_source = std::string(directive) + R"(

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec4 uTransform; // xy=center in NDC, zw=size in NDC
uniform vec4 uUVRegion;  // xy=uv offset, zw=uv size

out vec2 vUV;

void main() {
    vec2 pos = aPos * uTransform.zw + uTransform.xy;
    gl_Position = vec4(pos, 0.0, 1.0);
    vUV = uUVRegion.xy + aTexCoord * uUVRegion.zw;
}
)";

        std::string fs_source = std::string(directive) + R"(

in vec2 vUV;
out vec4 outColor;

uniform sampler2D uTexture;

void main() {
    outColor = texture(uTexture, vUV);
}
)";

        try
        {
            glState.shader = linkProgram(vs_source.c_str(), fs_source.c_str());
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[OpenGL] Pipeline build failed: " << ex.what() << "\n";
            destroy_quad_pipeline(glState);
            return false;
        }

        glState.uUVRegionLoc = glGetUniformLocation(glState.shader, "uUVRegion");
        glState.uTransformLoc = glGetUniformLocation(glState.shader, "uTransform");
        glState.uSamplerLoc = glGetUniformLocation(glState.shader, "uTexture");

        if (glState.uSamplerLoc >= 0)
        {
            glUseProgram(glState.shader);
            glUniform1i(glState.uSamplerLoc, 0);
            glUseProgram(0);
        }

        glGenVertexArrays(1, &glState.vao);
        glGenBuffers(1, &glState.vbo);
        glGenBuffers(1, &glState.ebo);

        if (!glState.vao || !glState.vbo || !glState.ebo)
        {
            std::cerr << "[OpenGL] Failed to allocate quad buffers\n";
            destroy_quad_pipeline(glState);
            return false;
        }

        glBindVertexArray(glState.vao);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vbo);

        constexpr float quadVerts[] = {
            -0.5f, -0.5f, 0.0f, 0.0f,
             0.5f, -0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(quadVerts)), quadVerts, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)),
            (void*)(2 * sizeof(float)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glState.ebo);
        constexpr unsigned int quadIdx[] = { 0, 1, 2, 2, 3, 0 };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(quadIdx)), quadIdx, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return true;
    }

    export bool ensure_quad_pipeline(almondnamespace::openglstate::OpenGL4State& glState)
    {
        const bool shaderValid = glState.shader != 0 && glIsProgram(glState.shader) == GL_TRUE;
        const bool vaoValid = glState.vao != 0 && glIsVertexArray(glState.vao) == GL_TRUE;
        const bool vboValid = glState.vbo != 0 && glIsBuffer(glState.vbo) == GL_TRUE;
        const bool eboValid = glState.ebo != 0 && glIsBuffer(glState.ebo) == GL_TRUE;

        if (shaderValid && vaoValid && vboValid && eboValid)
            return true;

        return build_quad_pipeline(glState);
    }

} // namespace almondnamespace::openglcontext

#endif // ALMOND_USING_OPENGL
