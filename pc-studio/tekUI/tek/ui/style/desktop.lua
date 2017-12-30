
local db = require "tek.lib.debug"
local getenv = os.getenv
local insert = table.insert
local ipairs = ipairs
local min = math.min
local open = io.open
local tonumber = tonumber
local floor = math.floor

local DesktopStyle = { }
DesktopStyle._VERSION = "Desktop Style 2.2"

-------------------------------------------------------------------------------
--	GTK+ settings import:
-------------------------------------------------------------------------------

local function fmtrgb(r, g, b, l)
	l = l or 1
	r = floor(min(r * l * 255, 255))
	g = floor(min(g * l * 255, 255))
	b = floor(min(b * l * 255, 255))
	return ("#%02x%02x%02x"):format(r, g, b)
end

local function rgb2yuv(R, G, B)
	local Y = 0.299 * R + 0.587 * G + 0.114 * B
	local U = (B - Y) * 0.493
	local V = (R - Y) * 0.877
	return Y, U, V
end

local function yuv2rgb(Y, U, V)
	local B = Y + U / 0.493
	local R = Y + V / 0.877
	local G = 1.704 * Y - 0.509 * R - 0.194 * B
	return R, G, B
end

local function splitrgb(rgb)
	local r, g, b = rgb:match("^%#?(%x%x)(%x%x)(%x%x)")
	return tonumber(r, 16) / 255, tonumber(g, 16) / 255, tonumber(b, 16) / 255
end

local function mixrgb(rgb1, rgb2, f)
	f = f or 1
	local r1, g1, b1 = splitrgb(rgb1)
	local r2, g2, b2 = splitrgb(rgb2)
	local y1, u1, v1 = rgb2yuv(r1, g1, b1)
	local y2, u2, v2 = rgb2yuv(r2, g2, b2)
	local y = (y1 + y2 * f) / (f + 1)
	local u = (u1 + u2 * f) / (f + 1)
	local v = (v1 + v2 * f) / (f + 1)
	local r, g, b = yuv2rgb(y, u, v)
	return fmtrgb(r, g, b), r, g, b
end

local function getclass(s, class)
	local c = s[class]
	if not c then
		c = { }
		s[class] = c
	end
	return c
end

local function setprop(d, key, val)
	if not d[key] then
		d[key] = val
	end
end

function DesktopStyle.importConfiguration(s)
	if 3 / 2 == 1 then
		db.error("Need floating point support for GTK+ settings import")
		return false
	end
	local p = getenv("GTK2_RC_FILES")
	if p then
		local paths = { }
		p:gsub("([^:]+):?", function(p)
			insert(paths, p)
		end)
		for _, fname in ipairs(paths) do
			db.info("Trying config file %s", fname)
			local f = open(fname)
			if f then
				local d = getclass(s, "tek.ui.class.display")
				local style
				local found = false
				for line in f:lines() do
					line = line:match("^%s*(.*)%s*$")
					local newstyle = line:match("^style%s+\"(%w+)\"$")
					if newstyle then
						style = newstyle:lower()
					end
					local color, r, g, b =
						line:match("^(%w+%[%w+%])%s*=%s*{%s*([%d.]+)%s*,%s*([%d.]+)%s*,%s*([%d.]+)%s*}$")
					if color and r then
						local r, g, b = tonumber(r), tonumber(g), tonumber(b)
						local c = fmtrgb(r, g, b)
						if style == "default" then
							found = true
							if color == "bg[NORMAL]" then
								setprop(d, "rgb-menu", c)
								setprop(d, "rgb-background", fmtrgb(r, g, b, 0.91))
								setprop(d, "rgb-group", fmtrgb(r, g, b, 0.985))
								setprop(d, "rgb-shadow", fmtrgb(r, g, b, 0.45))
								setprop(d, "rgb-border-shine", fmtrgb(r, g, b, 1.25))
								setprop(d, "rgb-border-shadow", fmtrgb(r, g, b, 0.65))
								setprop(d, "rgb-half-shine", fmtrgb(r, g, b, 1.1))
								setprop(d, "rgb-half-shadow", fmtrgb(r, g, b, 0.65))
								setprop(d, "rgb-outline", c)
								setprop(d, "rgb-shine", fmtrgb(r, g, b, 1.4))
								setprop(d, "rgb-border-rim", fmtrgb(r, g, b, 0.5))
								setprop(d, "rgb-disabled-detail", fmtrgb(r, g, b, 0.65))
								setprop(d, "rgb-disabled-detail-shine", fmtrgb(r, g, b, 1.1))
							elseif color == "bg[INSENSITIVE]" then
								setprop(d, "rgb-disabled", c)
							elseif color == "bg[ACTIVE]" then
								setprop(d, "rgb-active", c)
							elseif color == "bg[PRELIGHT]" then
								setprop(d, "rgb-hover", fmtrgb(r, g, b, 1.03))
								setprop(d, "rgb-focus", c)
							elseif color == "bg[SELECTED]" then
								setprop(d, "rgb-fill", c)
								setprop(d, "rgb-border-focus", c)
								setprop(d, "rgb-cursor", c)
							elseif color == "fg[NORMAL]" then
								setprop(d, "rgb-detail", c)
								setprop(d, "rgb-menu-detail", c)
								setprop(d, "rgb-border-legend", c)
							elseif color == "fg[INSENSITIVE]" then
								setprop(d, "rgb-disabled-detail", c)
								d["rgb-disabled-detail-shine"] = fmtrgb(r, g, b, 2)
							elseif color == "fg[ACTIVE]" then
								setprop(d, "rgb-active-detail", c)
							elseif color == "fg[PRELIGHT]" then
								setprop(d, "rgb-hover-detail", c)
								setprop(d, "rgb-focus-detail", c)
							elseif color == "fg[SELECTED]" then
								setprop(d, "rgb-cursor-detail", c)
							elseif color == "base[NORMAL]" then
								setprop(d, "rgb-list", fmtrgb(r, g, b, 1.05))
								setprop(d, "rgb-list-alt", fmtrgb(r, g, b, 0.92))
							elseif color == "base[SELECTED]" then
								setprop(d, "rgb-list-active", c)
							elseif color == "text[NORMAL]" then
								setprop(d, "rgb-list-detail", c)
							elseif color == "text[ACTIVE]" then
								setprop(d, "rgb-list-active-detail", c)
							end
						elseif style == "menuitem" then
							if color == "bg[NORMAL]" then
								setprop(d, "rgb-menu", c)
							elseif color == "bg[PRELIGHT]" then
								setprop(d, "rgb-menu-active", c)
							elseif color == "fg[NORMAL]" then
								setprop(d, "rgb-menu-detail", c)
							elseif color == "fg[PRELIGHT]" then
								setprop(d, "rgb-menu-active-detail", c)
							end
						end
					end
				end
				f:close()
				if found then
					if d["rgb-detail"] and d["rgb-background"] then
						d["rgb-border-legend"] = mixrgb(d["rgb-detail"], 
							d["rgb-background"], 0.63)
						d["rgb-caption-detail"] = mixrgb(d["rgb-detail"], 
							d["rgb-background"], 0.33)
					end
					return true
				end
			end
		end
	end
	return false
end

return DesktopStyle
