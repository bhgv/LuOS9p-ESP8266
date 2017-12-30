-------------------------------------------------------------------------------
--
--	tek.ui.image.arrowup
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	Version 1.4
--
-------------------------------------------------------------------------------

local ui = require "tek.ui"
local Image = ui.Image
local ArrowImage = Image.module("tek.ui.class.arrowup", "tek.ui.class.image")

local coords = { 0x1000,0x5000, 0xf000,0x5000, 0x8000,0xc000 }
local prims = { { 0x1000, 3, { 1, 2, 3 }, "detail" } }

function ArrowImage.new(class, num)
	return Image.new(class, { coords, false, false, true, prims })
end

return ArrowImage
