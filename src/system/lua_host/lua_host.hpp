/**
 * @file lua_host.hpp
 * @brief Embedded Lua runtime for MeowKit (plugin scripting engine).
 *
 * This is the foundation of the SD-based plugin system: a PSRAM-backed Lua VM
 * with a curated, embedded-safe standard library. Later phases layer the
 * `mk.*` device bindings (display, input, mic, audio, wifi, http/ws, ...) and
 * an App Runner that loads `/apps/<name>/main.lua` from the SD card on top of
 * the state created here.
 *
 * Phase 1 scope: bring the VM up and prove it runs on hardware. No bindings,
 * no SD loading yet.
 */
#pragma once

#include <Arduino.h>

extern "C" {
#include "lua.h"
}

namespace lua_host {

/**
 * @brief Create a fresh Lua state backed by PSRAM.
 *
 * The allocator routes every VM allocation to external SPIRAM, and a curated
 * set of standard libraries is opened: base, table, string, math, utf8,
 * coroutine. io/os/package/debug are intentionally left out so the VM cannot
 * reach system(), dynamic loaders, or raw files — those capabilities return
 * later as sandboxed mk.* bindings.
 *
 * @return a new lua_State the caller owns (close with lua_close), or nullptr
 *         if allocation failed.
 */
lua_State* newState();

/**
 * @brief One-shot bring-up check for the embedded VM.
 *
 * Spins up a state, runs a small script exercising tables/loops/strings, then
 * reports the computed value, the VM's memory footprint, and free PSRAM before
 * vs after (delta ~0 confirms the PSRAM allocator frees cleanly). Intended to
 * be printed once at boot during development.
 *
 * @return a single human-readable status line.
 */
String selfTest();

} // namespace lua_host
