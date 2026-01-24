///**************************************************************
// *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
// *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
// *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
// *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
// *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
// *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
// *                                                            *
// *   This file is part of the Almond Project.                 *
// *   AlmondShell - Modular C++ Framework                      *
// *                                                            *
// *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
// *                                                            *
// *   Provided "AS IS", without warranty of any kind.          *
// **************************************************************/
//#pragma once
//
//#if defined(_WIN32)
//
// // WGL requires GDI. Do NOT set NOGDI here.
//#ifndef WIN32_LEAN_AND_MEAN
//#  define WIN32_LEAN_AND_MEAN
//#endif
//#ifndef NOMINMAX
//#  define NOMINMAX
//#endif
//#ifndef _WINSOCKAPI_
//#  define _WINSOCKAPI_
//#endif
//#pragma once
//
//// Windows API defines a global function named CloseWindow(HWND).
//// raylib also defines CloseWindow() (no args) with C linkage.
//// C++ forbids overloading extern "C" functions, so including both headers
//// in the same translation unit produces C2733.
////
//// Strategy: rename the Win32 API CloseWindow declaration while including Windows.
//// If you ever need the Win32 API CloseWindow, call ::CloseWindowWin32.
//
//// Rename Win32 CloseWindow during include.
//#define CloseWindow CloseWindowWin32
//#include <windows.h>
//#undef CloseWindow
//
//#include <wingdi.h>
//
//// -----------------------------------------------------------------------------
//// IMPORTANT:
//// wglext.h (and glad_wgl.h) require OpenGL base types (GLenum/GLint/GLuint/etc).
//// Ensure glad.h (preferred) or gl.h is included BEFORE wglext/glad_wgl.
//// -----------------------------------------------------------------------------
//
//#if defined(__has_include)
//#  if __has_include(<glad/glad.h>)
//#    include <glad/glad.h>
//#  else
//#    include <GL/gl.h>
//#  endif
//#else
//#  include <glad/glad.h>
//#endif
//
//#if defined(__has_include)
//#  if __has_include(<glad/glad_wgl.h>)
//#    include <glad/glad_wgl.h>
//#  else
//#    include <GL/wglext.h>
//#  endif
//#else
//#  include <GL/wglext.h>
//#endif
//
//#endif // _WIN32
