-- nettest / main.lua — proves mk.wifi + mk.http can reach a backend.
--
-- The URL is read from /apps/nettest/url.txt on the SD card, so a real/private
-- endpoint lives on the card, never in the repo. Falls back to a public test
-- URL if that file is missing. Controls: A = GET, short B = back, hold B = exit.

local url    = "https://httpbin.org/get"
local status = nil
local body   = nil
local busy   = false
local last_a = false

local function load_url()
  local f = mk.fs.read("/apps/nettest/url.txt")
  if f then
    local u = f:gsub("%s+$", ""):gsub("^%s+", "")   -- trim
    if #u > 0 then url = u end
  end
end

local function draw()
  mk.display.clear(0x000000)
  mk.display.text(10, 8, "Net Test", 0xBBE700, 2)
  mk.display.text(10, 40, "wifi: " .. (mk.wifi.connected() and "up" or "down"), 0xFFFFFF, 1)
  mk.display.text(10, 56, "ip:   " .. mk.wifi.ip(),   0xBBBBBB, 1)
  mk.display.text(10, 72, "ssid: " .. mk.wifi.ssid(), 0xBBBBBB, 1)
  mk.display.text(10, 92, "url:", 0x888888, 1)
  mk.display.text(10, 104, url:sub(1, 50), 0x66CCFF, 1)

  if busy then
    mk.display.text(10, 132, "requesting...", 0xFFCC00, 1)
  elseif status ~= nil then
    mk.display.text(10, 132, "status: " .. tostring(status), 0x00FF66, 1)
    local b = body or ""
    for i = 0, 4 do
      local chunk = b:sub(i * 48 + 1, i * 48 + 48)
      if #chunk == 0 then break end
      mk.display.text(10, 148 + i * 13, chunk, 0xCCCCCC, 1)
    end
  end
  mk.display.text(10, 222, "A: GET    short B: back", 0x888888, 1)
end

function on_open()
  load_url()
  draw()
end

function on_running(dt)
  local a = mk.input.down("a")
  if a and not last_a and not busy then
    busy = true
    draw()                              -- show "requesting..." before we block
    local st, bd = mk.http.get(url)
    status = st and st or "ERR"
    body   = bd                         -- response body, or error string
    busy   = false
    draw()
    mk.sys.log("nettest GET " .. url .. " -> " .. tostring(status))
  end
  last_a = a
end
