
local ui = require "tek.ui".checkVersion(112)
local Element = ui.require("element", 16)

local DrawHook = Element.module("tek.ui.class.drawhook", "tek.ui.class.element")
DrawHook._VERSION = "DrawHook 3.2"

function DrawHook:layout(x0, y0, x1, y1)
end

function DrawHook:draw()
end

return DrawHook
