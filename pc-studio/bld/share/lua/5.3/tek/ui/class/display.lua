-------------------------------------------------------------------------------
--
--	tek.ui.class.display
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		Display ${subclasses(Display)}
--
--		This class manages a display.
--
--	ATTRIBUTES::
--		- {{AspectX [IG]}} (number)
--			- X component of the display's aspect ratio
--		- {{AspectY [IG]}} (number)
--			- Y component of the display's aspect ratio
--
--	IMPLEMENTS::
--		- Display:closeFont() - Closes font
--		- Display.createPixMap() - Creates a pixmap from picture file data
--		- Display:getFontAttrs() - Gets font attributes
--		- Display.getPaint() - Gets a a paint object, cached
--		- Display:colorToRGB() - Converts a color specification to RGB
--		- Display.getTime() - Gets system time
--		- Display:openFont() - Opens a named font
--		- Display.openDrawable() - Opens a drawable
--		- Display.sleep() - Sleeps for a period of time
--
--	STYLE PROPERTIES::
--		- {{font}}
--		- {{font-fixed}}
--		- {{font-large}}
--		- {{font-x-large}}
--		- {{font-xx-large}}
--		- {{font-menu}}
--		- {{font-small}}
--		- {{font-x-small}}
--		- {{font-xx-small}}
--		- {{rgb-active}}
--		- {{rgb-active-detail}}
--		- {{rgb-background}}
--		- {{rgb-border-focus}}
--		- {{rgb-border-legend}}
--		- {{rgb-border-rim}}
--		- {{rgb-border-shadow}}
--		- {{rgb-border-shine}}
--		- {{rgb-bright}}
--		- {{rgb-caption-detail}}
--		- {{rgb-cursor}}
--		- {{rgb-cursor-detail}}
--		- {{rgb-dark}}
--		- {{rgb-detail}}
--		- {{rgb-disabled}}
--		- {{rgb-disabled-detail}}
--		- {{rgb-disabled-detail-shine}}
--		- {{rgb-fill}}
--		- {{rgb-focus}}
--		- {{rgb-focus-detail}}
--		- {{rgb-group}}
--		- {{rgb-half-shadow}}
--		- {{rgb-half-shine}}
--		- {{rgb-hover}}
--		- {{rgb-hover-detail}}
--		- {{rgb-list}}
--		- {{rgb-list-active}}
--		- {{rgb-list-active-detail}}
--		- {{rgb-list-detail}}
--		- {{rgb-list-alt}}
--		- {{rgb-menu}}
--		- {{rgb-menu-active}}
--		- {{rgb-menu-active-detail}}
--		- {{rgb-menu-detail}}
--		- {{rgb-outline}}
--		- {{rgb-shadow}}
--		- {{rgb-shine}}
--		- {{rgb-user1}}
--		- {{rgb-user2}}
--		- {{rgb-user3}}
--		- {{rgb-user4}}
--
--	OVERRIDES::
--		- Class.new()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)

local Element = ui.require("element", 17)
local Visual = ui.loadLibrary("visual", 4)

local floor = math.floor
local open = io.open
local pairs = pairs
local tonumber = tonumber
local unpack = unpack or table.unpack

local Display = Element.module("tek.ui.class.display", "tek.ui.class.element")
Display._VERSION = "Display 33.7"

-------------------------------------------------------------------------------
--	Class data and constants:
-------------------------------------------------------------------------------

local DEF_RGB_BACK       = "#d2d2d2"
local DEF_RGB_DETAIL     = "#000"
local DEF_RGB_SHINE      = "#fff"
local DEF_RGB_FILL       = "#6e82a0"
local DEF_RGB_SHADOW     = "#777"
local DEF_RGB_HALFSHADOW = "#bebebe"
local DEF_RGB_HALFSHINE  = "#e1e1e1"
local DEF_RGB_FOCUS      = "#e05014"

local ColorDefaults =
{
	["background"] = DEF_RGB_BACK,
	["dark"] = DEF_RGB_DETAIL,
	["bright"] = DEF_RGB_SHINE,
	["outline"] = DEF_RGB_SHINE,
	["fill"] = DEF_RGB_FILL,
	["active"] = DEF_RGB_HALFSHADOW,
	["focus"] = DEF_RGB_BACK,
	["hover"] = DEF_RGB_HALFSHINE,
	["disabled"] = DEF_RGB_BACK,
	["detail"] = DEF_RGB_DETAIL,
	["caption-detail"] = DEF_RGB_DETAIL,
	["active-detail"] = DEF_RGB_DETAIL,
	["focus-detail"] = DEF_RGB_DETAIL,
	["hover-detail"] = DEF_RGB_DETAIL,
	["disabled-detail"] = DEF_RGB_SHADOW,
	["disabled-detail-shine"] = DEF_RGB_HALFSHINE,
	["border-shine"] = DEF_RGB_HALFSHINE,
	["border-shadow"] = DEF_RGB_SHADOW,
	["border-rim"] = DEF_RGB_DETAIL,
	["border-focus"] = DEF_RGB_FOCUS,
	["border-legend"] = DEF_RGB_DETAIL,
	["menu"] = DEF_RGB_BACK,
	["menu-detail"] = DEF_RGB_DETAIL,
	["menu-active"] = DEF_RGB_FILL,
	["menu-active-detail"] = DEF_RGB_SHINE,
	["list"] = DEF_RGB_BACK,
	["list-alt"] = DEF_RGB_HALFSHINE,
	["list-detail"] = DEF_RGB_DETAIL,
	["list-active"] = DEF_RGB_FILL,
	["list-active-detail"] = DEF_RGB_SHINE,
	["cursor"] = DEF_RGB_FILL,
	["cursor-detail"] = DEF_RGB_SHINE,
	["group"] = DEF_RGB_HALFSHADOW,
	["shadow"] = DEF_RGB_SHADOW,
	["shine"] = DEF_RGB_SHINE,
	["half-shadow"] = DEF_RGB_HALFSHADOW,
	["half-shine"] = DEF_RGB_HALFSHINE,
	["paper"] = DEF_RGB_SHINE,
	["ink"] = DEF_RGB_DETAIL,
	["user1"] = DEF_RGB_DETAIL,
	["user2"] = DEF_RGB_DETAIL,
	["user3"] = DEF_RGB_DETAIL,
	["user4"] = DEF_RGB_DETAIL,
	["black"] =   "#000",
	["red"] =     "#f00",
	["lime"] =    "#0f0",
	["yellow"] =  "#ff0",
	["blue"] =    "#00f",
	["fuchsia"] = "#f0f",
	["aqua"] =    "#0ff",
	["white"] =   "#fff",
	["gray"] =    "#808080",
	["maroon"] =  "#800000",
	["green"] =   "#008000",
	["olive"] =   "#808000",
	["navy"] =    "#000080",
	["purple"] =  "#800080",
	["teal"] =    "#008080",
	["silver"] =  "#c0c0c0",
	["orange"] =  "#ffa500",
}

-------------------------------------------------------------------------------
--	setup fonts:
--	style attributes (/b = bold, /i = italic, /bi = bold+italic)
--	are passed on to the display driver, which may or may not implement them.
--	Alternatively, mappings from styles to distinguished fontnames can be
--	placed in the FontsDefaults cache here.
-------------------------------------------------------------------------------

--local FN_FIXED = "Fantasque Sans Mono,VeraMono,DejaVuSansMono,monospace,fixed,courier new,courier,VeraMono"
--local FN_FIXEDBOLD = "Fantasque Sans Mono/b,VeraMono/b,DejaVuSansMono-Bold,DejaVuSansMono/b,monospace/b,fixed/b,courier new/b,courier/b,VeraMono/b"
--local FN_FIXEDITALIC = "Fantasque Sans Mono/i,VeraMono/i,DejaVuSansMono-Oblique,DejaVuSansMono/i,monospace/i,fixed/i,courier new/i,courier/i,VeraMono/i"
--local FN_FIXEDBOLDITALIC = "Fantasque Sans Mono/bi,VeraMono/bi,DejaVuSansMono-BoldOblique,DejaVuSansMono/bi,monospace/bi,fixed/bi,courier new/bi,courier/bi,VeraMono/bi"
local FN_FIXED = "DejaVuSansMono,monospace,fixed,courier new,courier,VeraMono"
local FN_FIXEDBOLD = "DejaVuSansMono-Bold,DejaVuSansMono/b,monospace/b,fixed/b,courier new/b,courier/b,VeraMono/b"
local FN_FIXEDITALIC = "DejaVuSansMono-Oblique,DejaVuSansMono/i,monospace/i,fixed/i,courier new/i,courier/i,VeraMono/i"
local FN_FIXEDBOLDITALIC = "DejaVuSansMono-BoldOblique,DejaVuSansMono/bi,monospace/bi,fixed/bi,courier new/bi,courier/bi,VeraMono/bi"

--local FN_NORMAL = "FantasqueSans,Vera,DejaVuSans,sans-serif,helvetica,arial,Vera,times"
--local FN_BOLD = "FantasqueSans/b,Vera/b,DejaVuSans-Bold,DejaVuSans/b,sans-serif/b,helvetica/b,arial/b,Vera/b,times/b"
--local FN_ITALIC = "FantasqueSans/i,Vera/i,DejaVuSans-Oblique,DejaVuSans/i,sans-serif/i,helvetica/i,arial/i,Vera/i,times/i"
--local FN_BOLDITALIC = "FantasqueSans/bi,Vera/bi,DejaVuSans-BoldOblique,DejaVuSans/bi,sans-serif/bi,helvetica/bi,arial/bi,Vera/bi,times/bi"
local FN_NORMAL = "DejaVuSans,sans-serif,helvetica,arial,Vera,times"
local FN_BOLD = "DejaVuSans-Bold,DejaVuSans/b,sans-serif/b,helvetica/b,arial/b,Vera/b,times/b"
local FN_ITALIC = "DejaVuSans-Oblique,DejaVuSans/i,sans-serif/i,helvetica/i,arial/i,Vera/i,times/i"
local FN_BOLDITALIC = "DejaVuSans-BoldOblique,DejaVuSans/bi,sans-serif/bi,helvetica/bi,arial/bi,Vera/bi,times/bi"

local fontsizes = 
{ 
	["xx-small"] = 8,
	["x-small"] = 9,
	["small"] = 11,
	["main"] = 13,
	["menu"] = 13,
	["large"] = 16,
	["x-large"] = 21,
	["xx-large"] = 28,
}

local fontstyles =
{
	[""] = FN_NORMAL,
	["/i"] = FN_ITALIC,
	["/b"] = FN_BOLD,
	["/bi"] = FN_BOLDITALIC
}

local FontDefaults =
{
	-- cache name : propname : default
	["ui-fixed"] = { "font-fixed", FN_FIXED .. ":" .. fontsizes["main"] },
	["ui-fixed/b"] = { false, FN_FIXEDBOLD .. ":" .. fontsizes["main"] },
	["ui-fixed/i"] = { false, FN_FIXEDITALIC .. ":" .. fontsizes["main"] },
	["ui-fixed/bi"] = { false, FN_FIXEDBOLDITALIC .. ":" .. fontsizes["main"] },
	["ui-icons"] = { false, "Icons" .. ":" .. fontsizes["main"] },
}

for szname, size in pairs(fontsizes) do
	for attr, fname in pairs(fontstyles) do
		local key = "ui-" .. szname .. attr -- e.g. "ui-small/b"
		local val = fname .. ":" .. size -- e.g. "dejavusans-bold,arial/b:11"
		local propname = attr == "" and "font-"..szname or ""
-- 		db.warn("cache-key:%s propname:%s default:%s", key, propname, val)
		FontDefaults[key] = { propname, val }
	end
end

FontDefaults[""] = FontDefaults["ui-main"] -- default
FontDefaults["ui-huge"] = FontDefaults["ui-xx-large"] -- alias name (no styles)

-------------------------------------------------------------------------------

local PixmapCache = { }

-------------------------------------------------------------------------------
--	image, width, height, transparency = Display.createPixmap(picture):
--	Creates a pixmap object from a picture file or from a table.
--	See Visual.createPixmap().
-------------------------------------------------------------------------------

Display.createPixmap = Visual.createPixmap
Display.createGradient = Visual.createGradient

-------------------------------------------------------------------------------

Display.getDisplayAttrs = Visual.getDisplayAttrs

-------------------------------------------------------------------------------
--	image, width, height, transparency = Display.getPaint(imgspec):
--	Gets a paint object from a specifier, either by loading it from the
--	filesystem, generating it, or by retrieving it from the cache. To resolve
--	symbolic color names, a display instance must be given.
-------------------------------------------------------------------------------

function Display.getPaint(imgspec, display, width, height)
	if PixmapCache[imgspec] then
		db.trace("got cache copy for '%s'", imgspec)
		return unpack(PixmapCache[imgspec])
	end
	local imgtype, location = imgspec:match("^(%a+)%((.+)%)")
	local paint, w, h, trans
	if imgtype == "url" then
		local f = open(location, "rb")
		if f then
			paint, w, h, trans = Display.createPixmap(f, width, height)
			f:close()
		else
			db.warn("cannot load image '%s'", location)
		end
	elseif imgtype == "gradient" then
		local x0, y0, c0, x1, y1, c1 = 
			location:match("^(%-?%d+),(%-?%d+),(%S+),(%-?%d+),(%-?%d+),(%S+)$")
		local _, r0, g0, b0
		x0, y0, x1, y1 = tonumber(x0), tonumber(y0), tonumber(x1), tonumber(y1)
		if x0 and y0 and x1 and y1 then
			if display then
				_, r0, g0, b0 = display:colorToRGB(c0)
				_, r1, g1, b1 = display:colorToRGB(c1)
			else
				_, r0, g0, b0 = hexToRGB(c0)
				_, r1, g1, b1 = hexToRGB(c1)
			end
			if r0 and r1 then
				local rgb0 = r0 * 65536 + g0 * 256 + b0
				local rgb1 = r1 * 65536 + g1 * 256 + b1
				paint = Display.createGradient(x0, y0, x1, y1, rgb0, rgb1)
			end
		else
			db.error("Invalid gradient arguments: '%s'", location)
		end
	end
	if paint then
		PixmapCache[imgspec] = { paint, w, h, trans }
	end
	return paint, w, h, trans
end

-------------------------------------------------------------------------------
--	a, r, g, b = hexToRGB(colspec) - Converts a hexadecimal string color
--	specification to RGB and an opacity in the range from 0 to 255.
--	Valid color specifications are {{#rrggbb}}, {{#aarrggbb}} (each component
--	is noted in 8 bit hexadecimal) and {{#rgb}} (each color component is noted
--	in 4 bit hexadecimal).
-------------------------------------------------------------------------------

function Display.hexToRGB(col)
	local r, g, b = col:match("^%s*%#(%x%x)(%x%x)(%x%x)%s*$")
	if r then
		return 255, tonumber(r, 16), tonumber(g, 16), tonumber(b, 16)
	end
	r, g, b = col:match("^%s*%#(%x)(%x)(%x)%s*$")
	if r then
		r, g, b = tonumber(r, 16), tonumber(g, 16), tonumber(b, 16)
		return 255, r * 16 + r, g * 16 + g, b * 16 + b
	end
	local a
	a, r, g, b = col:match("^%s*%#(%x%x)(%x%x)(%x%x)(%x%x)%s*$")
	if a then
		return tonumber(r, 16), tonumber(r, 16), tonumber(g, 16), 
			tonumber(b, 16)
	end
end

-------------------------------------------------------------------------------
--	wait:
-------------------------------------------------------------------------------

Display.wait = Visual.wait

-------------------------------------------------------------------------------
--	getMsg:
-------------------------------------------------------------------------------

Display.getMsg = Visual.getMsg

-------------------------------------------------------------------------------
--	sleep:
-------------------------------------------------------------------------------

Display.sleep = Visual.sleep

-------------------------------------------------------------------------------
--	Display.getTime(): Gets the system time.
-------------------------------------------------------------------------------

Display.getTime = Visual.getTime

-------------------------------------------------------------------------------
--	Display.openDrawable(): Open a drawable
-------------------------------------------------------------------------------

Display.openDrawable = Visual.open

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function Display.new(class, self)
	self = self or { }
	self.AspectX = self.AspectX or 1
	self.AspectY = self.AspectY or 1
	self.FontCache = { }
	return Element.new(class, self)
end

-------------------------------------------------------------------------------
--	w, h = fitMinAspect(w, h, iw, ih[, evenodd]) - Fit to size, considering
--	the display's aspect ratio. If the optional {{evenodd}} is {{0}}, even
--	numbers are returned, if it is {{1}}, odd numbers are returned.
-------------------------------------------------------------------------------

function Display:fitMinAspect(w, h, iw, ih, round)
	local ax, ay = self.AspectX, self.AspectY
	if w * ih * ay / (ax * iw) > h then
		w = h * ax * iw / (ay * ih)
	else
		h = w * ih * ay / (ax * iw)
	end
	if round then
		return floor(w / 2) * 2 + round, floor(h / 2) * 2 + round
	end
	return floor(w), floor(h)
end

-------------------------------------------------------------------------------
--	a, r, g, b = colorToRGB(key): Gets the r, g, b values of a color. The color
--	can be a hexadecimal RGB specification or a symbolic name.
-------------------------------------------------------------------------------

function Display:colorToRGB(key)
	local props = self.Properties
	local col = props["rgb-" .. key] or ColorDefaults[key]
	if not col then
		local key1, key2, f = key:match("^tint%((%S+),(%S+),(%S+)%%%)$")
		if key1 then
			local a1, r1, g1, b1 = self.hexToRGB(props["rgb-" .. key1] or ColorDefaults[key1] or key1)
			local a2, r2, g2, b2 = self.hexToRGB(props["rgb-" .. key2] or ColorDefaults[key2] or key2)
			if a1 and a2 then
				local a = floor(a1 + (a2 - a1) * f / 100)
				local r = floor(r1 + (r2 - r1) * f / 100)
				local g = floor(g1 + (g2 - g1) * f / 100)
				local b = floor(b1 + (b2 - b1) * f / 100)
				return a, r, g, b
			end
		end
	end
	return self.hexToRGB(col or key)
end

-------------------------------------------------------------------------------
--	font = openFont(fontspec[, size[, attr]]): Opens a font, cached, and with
--	complex name resolution and fallbacks. For a discussion of the format
--	see [[#tek.ui.class.text : Text]]. The optional size and attr arguments
--	allow to override their respective values in the fontspec argument.
-------------------------------------------------------------------------------

function Display:openFont(fontspec, override_size, override_attr)
	fontspec = fontspec or ""
	local fcache = self.FontCache
	if not override_size and not override_attr then
		local frec = fcache[fontspec]
		if frec and frec[1] then
			return frec[1], frec
		end
	end
	local orig_fontspec = fontspec
	local fnames, size = fontspec:match("^%s*([^:]*)%s*%:?%s*(%d*)%s*$")
	if override_size then
		fontspec = fnames .. ":" .. override_size
		size = override_size
	else
		size = tonumber(size)
	end
	local props = self.Properties
	for fname in (fnames or ""):gmatch("%s*([^,]*)%s*,?") do
		local realname, attr = fname:match("^([^/]*)/?(.*)$")
		if override_attr then
			attr = override_attr
		end
		local fcachename = fname
		if size then
			fcachename = fcachename .. ":" .. size
		end
		local font
		local defrec = FontDefaults[fname]
		if defrec then
			local aliases = props[defrec[1]] or defrec[2]
			if aliases and aliases ~= orig_fontspec then
				font = self:openFont(aliases, size, attr ~= "" and attr)
			end
		end
		db.info("trying font %s:%s/%s", realname, size, attr)
		font = font or Visual.openFont(realname, size, attr .. "s")
		local frec = { font, font and font:getFontAttrs { } }
		fcache[fontspec] = frec
		fcache[fcachename] = frec
		if font then
			db.info("opened font %s:%s/%s", realname, size or "", attr)
			fcache[font] = frec
			return font, frec
		end
	end
	db.error("failed to open font '%s'", fontspec)
end

-------------------------------------------------------------------------------
--	closeFont(font): Closes the specified font. Always returns '''false'''.
-------------------------------------------------------------------------------

function Display:closeFont(font)
	-- do nothing here, keep them cached, let gc take care of it
	return false
end

-------------------------------------------------------------------------------
--	h, up, ut = getFontAttrs(font): Returns the font attributes height,
--	underline position and underline thickness.
-------------------------------------------------------------------------------

function Display:getFontAttrs(font)
	local a = self.FontCache[font][2]
	return a.Height, a.UlPosition, a.UlThickness
end

-------------------------------------------------------------------------------
--	flushCaches()
-------------------------------------------------------------------------------

function Display:flushCaches()
	PixmapCache = { }
	self.FontCache = { }
end

return Display
