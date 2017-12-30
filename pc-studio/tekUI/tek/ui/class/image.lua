
--
--	tek.ui.class.image
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		Implements bitmap and vector images
--

local Class = require "tek.class"

local ui = require "tek.ui".checkVersion(112)
local Display = ui.require("display", 21)

local assert = assert
local type = type

local Image = Class.module("tek.ui.class.image", "tek.class")
Image._VERSION = "Image 3.1"

function Image.new(class, image)
	if type(image) == "string" then
		image = { Display.createPixmap(image) }
	end
	assert(image[1])
	image[2] = image[2] or false -- width (false: stretchable)
	image[3] = image[3] or false -- height (false: stretchable)
	image[4] = image[4] or false -- transparent? (false: opaque)
	image[5] = image[5] or false -- vector primitives (false: is a pixmap)
	return Class.new(class, image)
end

function Image:draw(d, r1, r2, r3, r4, pen)
	if self[5] then
		d:drawImage(self, r1, r2, r3, r4, pen)
	else
		d:drawPixmap(self[1], r1, r2, r3, r4)
	end
end

function Image:askWidthHeight(w, h)
	return self[2] or w, self[3] or h
end

function Image:getPixmap()
	if not self[5] then
		return self[1], self[2], self[3], self[4]
	end
end

return Image
