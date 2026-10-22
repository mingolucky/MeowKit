-- bounce / main.lua — dt-based animation demo.
-- Shows smooth motion (no full-screen clear → no flicker), joystick input,
-- and the mk.display primitives. Controls: joystick nudges the ball,
-- A = red, short B = back to list, hold B = exit.

local W, H   = mk.display.width(), mk.display.height()
local r      = 8
local x, y   = 60, 60
local px, py = x, y
local vx, vy = 95, 72          -- pixels / second
local col    = 0x00FF66

function on_open()
  W, H = mk.display.width(), mk.display.height()
  mk.display.clear(0x000000)
  mk.display.text(6, 4, "Bounce (Lua)", 0xBBE700, 1)
end

function on_running(dt)
  local s = dt / 1000.0
  if s > 0.1 then s = 0.1 end   -- clamp after a stall

  x = x + vx * s
  y = y + vy * s

  if x < r         then x = r;         vx = -vx end
  if x > W - r     then x = W - r;     vx = -vx end
  if y < r + 18    then y = r + 18;    vy = -vy end
  if y > H - r     then y = H - r;     vy = -vy end

  if mk.input.down("left")  then vx = vx - 8 end
  if mk.input.down("right") then vx = vx + 8 end
  if mk.input.down("up")    then vy = vy - 8 end
  if mk.input.down("down")  then vy = vy + 8 end
  col = mk.input.down("a") and 0xFF5555 or 0x00FF66

  -- Erase previous ball, draw new one — cheap and flicker-free.
  mk.display.circle(math.floor(px), math.floor(py), r, 0x000000, true)
  mk.display.circle(math.floor(x),  math.floor(y),  r, col,      true)
  px, py = x, y
end
