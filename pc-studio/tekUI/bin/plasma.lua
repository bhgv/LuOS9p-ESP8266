#!/usr/bin/env lua

local visual = require "tek.lib.visual"

local COS = math.cos
local SIN = math.sin
local PI = math.pi
local MIN = math.min
local MAX = math.max
local FLOOR = math.floor

local WIDTH = 80
local HEIGHT = 60
local PIXWIDTH = 6
local PIXHEIGHT = 6

local WW = WIDTH * PIXWIDTH
local WH = HEIGHT * PIXHEIGHT

--
--	open window
--

local v = visual.open { Title = "Plasma", Width = WW, Height = WH,
	MinWidth = WW, MinHeight = WH, MaxWidth = WW, MaxHeight = WH }

-- react on close, keydown, interval; see also tek.ui
v:setInput(0x0901)

--
--	init
--

local screen = { }
local palette = { }
local palindex = 0

function addgradient(sr, sg, sb, dr, dg, db, num)
	dr = (dr - sr) / (num - 1)
	dg = (dg - sg) / (num - 1)
	db = (db - sb) / (num - 1)
	for i = 0, num - 1 do
		palette[palindex] = FLOOR(sr) * 65536 + FLOOR(sg) * 256 + FLOOR(sb)
		palindex = palindex + 1
		sr = sr + dr
		sg = sg + dg
		sb = sb + db
	end
end

addgradient(209,219,155, 79,33,57, 68)
addgradient(79,33,57, 209,130,255, 60)

local sintab = { }
for i = 0, 1023 do
	sintab[i] = SIN(i / 1024 * PI * 2)
end

--
--	effect
--

local xp1, xp2 = 0, 0
local yp1, yp2, yp3 = 0, 0, 0

function effect()

	local palettescale = #palette / 10
	local yc1, yc2, yc3 = yp1, yp2, yp3
	local i = 0

	for y = 0, HEIGHT - 1 do

		local xc1, xc2 = xp1, xp2
		local ysin = sintab[yc1] + sintab[yc2] + sintab[yc3] + 5

		for x = i, i + WIDTH - 1 do

			local c = sintab[xc1] + sintab[xc2] + ysin
			screen[x] = palette[FLOOR(c * palettescale)]
			xc1 = (xc1 - 12) % 1024
			xc2 = (xc2 + 13) % 1024

		end
		i = i + WIDTH

		yc1 = (yc1 + 8) % 1024
		yc2 = (yc2 + 11) % 1024
		yc3 = (yc3 - 18) % 1024

	end

	yp1 = (yp1 - 9) % 1024
	yp2 = (yp2 + 4) % 1024
	yp3 = (yp3 + 5) % 1024

	xp1 = (xp1 + 7) % 1024
	xp2 = (xp2 - 2) % 1024

end

--
--		Test default font capability
--

local tw, th, pen
local font = visual.openFont() -- open default font, if available
if font then
	pen = v:allocPen(0,255,255,255)
	tw, th = font:getTextSize("Plasma")
	v:setFont(font)
end

--
--	main loop
--

local abort, paint
repeat
	visual.wait()
	repeat
		local msg = visual.getMsg()
		if msg then
			local typ, code, mx, my = msg[2], msg[3], msg[4], msg[5]
			if typ == 1 or typ == 0x8000 -- closewindow or abort
				or (typ == 256 and code == 27) then -- escape key
				abort = true
			elseif typ == 2048 then -- interval
				paint = true
			end
		end
	until not msg
	if paint then
		effect()
		v:drawRGB(0, 0, screen, WIDTH, HEIGHT, PIXWIDTH, PIXHEIGHT)
		if font then
			v:drawText(0, 0, tw, th, "Plasma", pen)
		end
		v:flush()
		paint = false
	end
until abort
