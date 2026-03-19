-- basic.lua: utility functions for EzyCad Lua console

local function log_line(line, prefix)
  local s = (prefix or "") .. line
  if ezy and ezy.log then
    ezy.log(s)
  else
    print(s)
  end
end

--- kv: dump keys and values of a Lua object (table). For userdata, dumps metatable __index (available methods).
--- For other non-tables, outputs the value.
--- Uses ezy.log() when available so output appears in the Lua console.
--- @param obj any Lua value (table = dump keys/values; userdata = list methods; other = single line)
--- @param prefix string optional indent prefix for nested tables
function kv(obj, prefix)
  prefix = prefix or ""
  if type(obj) == "userdata" then
    log_line(tostring(obj), prefix)
    local mt = getmetatable(obj)
    if mt and type(mt) == "table" then
      local idx = mt.__index
      if type(idx) == "table" then
        log_line("  (methods)", prefix)
        for k, v in pairs(idx) do
          log_line("  " .. tostring(k) .. " = " .. tostring(v), prefix)
        end
      else
        -- fallback: show metatable keys (e.g. __index = function)
        for k, v in pairs(mt) do
          log_line("  " .. tostring(k) .. " = " .. tostring(v), prefix)
        end
      end
    end
    return
  end
  if type(obj) ~= "table" then
    log_line(tostring(obj), prefix)
    return
  end
  for k, v in pairs(obj) do
    log_line(tostring(k) .. " = " .. tostring(v), prefix)
    if type(v) == "table" and v ~= obj then
      kv(v, prefix .. "  ")
    end
  end
end
