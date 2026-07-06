-- Sierpinski triangle sketch generator for EzyCad Lua console.
-- Just for fun!
--
-- Usage (runs on startup, or call from the console):
--   create_sierpinski(3, 20.0)           -- log nodes and segments
--   create_sierpinski_sketch(3, 20.0)    -- build as a new sketch

local function mid(a, b)
  return { (a[1] + b[1]) / 2.0, (a[2] + b[2]) / 2.0 }
end

local function append_sierpinski_segments(order, p1, p2, p3, lines)
  if order <= 0 then
    lines[#lines + 1] = { p1, p2 }
    lines[#lines + 1] = { p2, p3 }
    lines[#lines + 1] = { p3, p1 }
    return
  end
  local m12 = mid(p1, p2)
  local m23 = mid(p2, p3)
  local m31 = mid(p3, p1)
  append_sierpinski_segments(order - 1, p1, m12, m31, lines)
  append_sierpinski_segments(order - 1, p2, m23, m12, lines)
  append_sierpinski_segments(order - 1, p3, m31, m23, lines)
end

local function line_key(a, b)
  if a[1] < b[1] or (a[1] == b[1] and a[2] < b[2]) then
    return string.format("%.9g,%.9g|%.9g,%.9g", a[1], a[2], b[1], b[2])
  end
  return string.format("%.9g,%.9g|%.9g,%.9g", b[1], b[2], a[1], a[2])
end

function create_sierpinski(order, size, center)
  order = order or 3
  size = size or 20.0
  center = center or { 0.0, 0.0 }
  local cx, cy = center[1], center[2]
  local h = size * math.sqrt(3.0) / 2.0
  local p1 = { cx - size / 2.0, cy - h / 3.0 }
  local p2 = { cx + size / 2.0, cy - h / 3.0 }
  local p3 = { cx, cy + 2.0 * h / 3.0 }

  local lines = {}
  append_sierpinski_segments(order, p1, p2, p3, lines)

  local seen = {}
  local unique_lines = {}
  for _, seg in ipairs(lines) do
    local a, b = seg[1], seg[2]
    local key = line_key(a, b)
    if not seen[key] then
      seen[key] = true
      unique_lines[#unique_lines + 1] = { a, b }
    end
  end

  local node_map = {}
  for _, seg in ipairs(unique_lines) do
    node_map[string.format("%.9g,%.9g", seg[1][1], seg[1][2])] = seg[1]
    node_map[string.format("%.9g,%.9g", seg[2][1], seg[2][2])] = seg[2]
  end
  local node_list = {}
  for _, pt in pairs(node_map) do
    node_list[#node_list + 1] = pt
  end
  table.sort(node_list, function(u, v)
    if u[2] ~= v[2] then return u[2] < v[2] end
    return u[1] < v[1]
  end)

  ezy.log(string.format("Sierpinski triangle: order=%d, size=%g, center=(%g, %g)", order, size, cx, cy))
  ezy.log(string.format("  unique nodes: %d", #node_list))
  for i, pt in ipairs(node_list) do
    ezy.log(string.format("    [%d] (%.6f, %.6f)", i, pt[1], pt[2]))
  end
  ezy.log(string.format("  line segments: %d", #unique_lines))
  for i, seg in ipairs(unique_lines) do
    local a, b = seg[1], seg[2]
    ezy.log(string.format("    [%d] (%.6f,%.6f) -> (%.6f,%.6f)", i, a[1], a[2], b[1], b[2]))
  end
  ezy.log("To build the sketch: create_sierpinski_sketch(order, size, center)")

  sierpinski_nodes = node_list
  sierpinski_lines = unique_lines
  return node_list, unique_lines
end

function create_sierpinski_sketch(order, size, center, plane, offset, name)
  order = order or 3
  size = size or 20.0
  center = center or { 0.0, 0.0 }
  plane = plane or "XY"
  offset = offset or 0.0
  name = name or "Sierpinski"
  local node_list, unique_lines = create_sierpinski(order, size, center)
  view.add_sketch(plane, offset, name)
  for _, seg in ipairs(unique_lines) do
    local a, b = seg[1], seg[2]
    view.add_edge(a[1], a[2], b[1], b[2])
  end
  view.finish_sketch_edges()
  ezy.log(string.format("Created sketch '%s' with %d edges.", view.curr_sketch_name(), #unique_lines))
  return node_list, unique_lines
end

-- Small default on load so output is visible without spamming.
local ok, err = pcall(function()
  create_sierpinski(2, 10.0)
end)
if not ok then
  ezy.log("sierpinski auto-gen error: " .. tostring(err))
end
