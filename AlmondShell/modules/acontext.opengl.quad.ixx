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

import <utility>;

#if defined(ALMOND_USING_OPENGL)

export namespace almondnamespace::openglcontext
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
} // namespace almondnamespace::openglcontext

#endif // ALMOND_USING_OPENGL
