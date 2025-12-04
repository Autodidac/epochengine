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
// aplatform.hpp
#pragma once

#include <string>
#include <ctime>

// Define the namespace based on a macro
#define almondnamespace almondshell

// API Visibility Macros
#ifdef _WIN32
    // Windows DLL export/import logic
#ifndef ENGINE_STATICLIB
#ifdef ENGINE_DLL_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#else
#define ENGINE_API  // Static library, no special attributes
#endif
#else
    // GCC/Clang visibility attributes
#if (__GNUC__ >= 4) && !defined(ENGINE_STATICLIB) && defined(ENGINE_DLL_EXPORTS)
#define ENGINE_API __attribute__((visibility("default")))
#else
#define ENGINE_API  // Static library or unsupported compiler
#endif
#endif

// Calling Convention Macros
#ifndef _STDCALL_SUPPORTED
#define ALECALLCONV __cdecl
#else
#define ALECALLCONV __stdcall
#endif

#ifdef _WIN32
#ifndef ALMOND_MAIN_HEADLESS
#include <shellscalingapi.h>  // For GetDpiForMonitor
#pragma comment(lib, "Shcore.lib")
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")
#endif // !ALMOND_MAIN_HEADLESS

// <ctime> (via <time.h>) on MSVC defines a macro named `time` that collides with
// our `almondnamespace::time` namespace. Undefine it after the Windows headers so
// nested namespace usage stays intact.
#ifdef time
#undef time
#endif // !ALMOND_MAIN_HEADLESS
#endif