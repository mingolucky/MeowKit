-- hello / main.lua — minimal MeowKit Lua plugin.
-- Demonstrates the mk.display / mk.input / mk.time / mk.sys bindings.
--
-- Lifecycle globals (all optional): on_open(), on_running(dt_ms), on_close().
-- Controls: A = change colour · short B = back to list · hold B = exit.

local frames  = 0
local palette = { 0x00FF66, 0xFFCC00, 0x33AAFF, 0xFF5555 }
local ci      = 1
local last_a  = false
local last_ms = 0

function on_open()
  mk.sys.log("hello: on_open")
end

function on_running(dt)
  frames = frames + 1

  -- A cycles the accent colour (poll the level, detect our own edge).
  local a = mk.input.down("a")
  if a and not last_a then ci = ci % #palette + 1 end
  last_a = a

  -- Redraw ~20 fps to keep flicker and CPU down.
  local now = mk.time.ms()
  if now - last_ms < 50 then return end
  last_ms = now

  mk.display.clear(0x000000)
  mk.display.text(10, 10, "Hello from Lua!", palette[ci], 2)
  mk.display.text(10, 48, "frames: " .. frames,        0xBBBBBB, 1)
  mk.display.text(10, 64, "uptime: " .. now .. " ms",  0xBBBBBB, 1)
  mk.display.text(10, 80, "heap:   " .. mk.sys.heap()  .. " B", 0xBBBBBB, 1)
  mk.display.text(10, 96, "psram:  " .. mk.sys.psram() .. " B", 0xBBBBBB, 1)

  mk.display.rect(10, 124, 300, 2, palette[ci])
  mk.display.text(10, 150, "A: change colour",      0x888888, 1)
  mk.display.text(10, 166, "short B: back to list", 0x888888, 1)
  mk.display.text(10, 182, "hold  B: exit to home", 0x888888, 1)
end

function on_close()
  mk.sys.log("hello: on_close")
end
