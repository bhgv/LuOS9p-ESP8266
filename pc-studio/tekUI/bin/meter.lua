#!/usr/bin/env lua

local floor = math.floor
ui = require "tek.ui"

-------------------------------------------------------------------------------
--	Sine table, 256 entries in 16 bit:
-------------------------------------------------------------------------------

local sintab =
{
	0x7fff,0x8326,0x864d,0x8972,0x8c97,0x8fb9,0x92d9,0x95f6,
	0x9910,0x9c25,0x9f37,0xa243,0xa54a,0xa84c,0xab47,0xae3b,
	0xb128,0xb40e,0xb6eb,0xb9c0,0xbc8c,0xbf4f,0xc208,0xc4b6,
	0xc75a,0xc9f2,0xcc80,0xcf01,0xd176,0xd3de,0xd639,0xd887,
	0xdac8,0xdcfa,0xdf1d,0xe132,0xe338,0xe52e,0xe714,0xe8eb,
	0xeab1,0xec67,0xee0c,0xef9f,0xf122,0xf292,0xf3f1,0xf53e,
	0xf679,0xf7a1,0xf8b7,0xf9ba,0xfaaa,0xfb87,0xfc50,0xfd07,
	0xfdaa,0xfe39,0xfeb5,0xff1d,0xff72,0xffb2,0xffdf,0xfff8,
	0xfffd,0xffee,0xffcb,0xff94,0xff4a,0xfeec,0xfe7a,0xfdf4,
	0xfd5b,0xfcae,0xfbee,0xfb1b,0xfa34,0xf93b,0xf82e,0xf70f,
	0xf5de,0xf49a,0xf344,0xf1dc,0xf063,0xeed8,0xed3b,0xeb8e,
	0xe9d0,0xe802,0xe623,0xe435,0xe237,0xe029,0xde0d,0xdbe2,
	0xd9a9,0xd762,0xd50d,0xd2ac,0xd03d,0xcdc2,0xcb3a,0xc8a7,
	0xc609,0xc360,0xc0ac,0xbdef,0xbb28,0xb857,0xb57e,0xb29c,
	0xafb3,0xacc2,0xa9ca,0xa6cc,0xa3c8,0xa0be,0x9daf,0x9a9b,
	0x9783,0x9468,0x9149,0x8e28,0x8b05,0x87e0,0x84b9,0x8192,
	0x7e6b,0x7b44,0x781d,0x74f8,0x71d5,0x6eb4,0x6b95,0x687a,
	0x6562,0x624e,0x5f3f,0x5c35,0x5931,0x5633,0x533b,0x504a,
	0x4d61,0x4a7f,0x47a6,0x44d5,0x420e,0x3f51,0x3c9d,0x39f4,
	0x3756,0x34c3,0x323b,0x2fc0,0x2d51,0x2af0,0x289b,0x2654,
	0x241b,0x21f0,0x1fd4,0x1dc6,0x1bc8,0x19da,0x17fb,0x162d,
	0x146f,0x12c2,0x1125,0x0f9a,0x0e21,0x0cb9,0x0b63,0x0a1f,
	0x08ee,0x07cf,0x06c2,0x05c9,0x04e2,0x040f,0x034f,0x02a2,
	0x0209,0x0183,0x0111,0x00b3,0x0069,0x0032,0x000f,0x0000,
	0x0005,0x001e,0x004b,0x008b,0x00e0,0x0148,0x01c4,0x0253,
	0x02f6,0x03ad,0x0476,0x0553,0x0643,0x0746,0x085c,0x0984,
	0x0abf,0x0c0c,0x0d6b,0x0edb,0x105e,0x11f1,0x1396,0x154c,
	0x1712,0x18e9,0x1acf,0x1cc5,0x1ecb,0x20e0,0x2303,0x2535,
	0x2776,0x29c4,0x2c1f,0x2e87,0x30fc,0x337d,0x360b,0x38a3,
	0x3b47,0x3df5,0x40ae,0x4371,0x463d,0x4912,0x4bef,0x4ed5,
	0x51c2,0x54b6,0x57b1,0x5ab3,0x5dba,0x60c6,0x63d8,0x66ed,
	0x6a07,0x6d24,0x7044,0x7366,0x768b,0x79b0,0x7cd7,0x7ffe,
}

-------------------------------------------------------------------------------
--	Derive a graph class that reacts on messages of the type MSG_USER for
--	updating its curve:
-------------------------------------------------------------------------------

local MyGraph1 = ui.Meter:newClass()

function MyGraph1:show()
	ui.Meter.show(self)
	self.Application:addInputHandler(ui.MSG_USER, self, self.updateData)
end

function MyGraph1:hide()
	self.Application:remInputHandler(ui.MSG_USER, self, self.updateData)
	ui.Meter.hide(self)
end

function MyGraph1:updateData(msg)
	local i = 0
	local c = self.Curves[2]
	for val in msg[-1]:gmatch("(%d+)") do
		i = i + 1
		c[i] = tonumber(val)
	end
	self.RedrawGraph = true
	self:setFlags(ui.FL_CHANGED)
	return msg
end

function MyGraph1:drawGraph()
	local g = self.GraphRect
	local w = g[3] - g[1] + 1
	local h = g[4] - g[2] + 1
	local d = self.Window.Drawable
	d:drawLine(floor(g[1]), floor(g[2] + h/2), floor(g[3]), floor(g[2] + h/2),
		"user4")
	d:drawLine(floor(g[1] + w/2), floor(g[2]), floor(g[1] + w/2), floor(g[4]),
		"user4")
	ui.Meter.drawGraph(self)
end

-------------------------------------------------------------------------------
--	Derive a graph class that produces a sinetable effect:
-------------------------------------------------------------------------------

local MyGraph2 = ui.Meter:newClass()

function MyGraph2.new(class, self)
	self = self or { }
	self.Params = { 0,0,0 }
	return ui.Meter.new(class, self)
end

function MyGraph2:show(drawable)
	ui.Meter.show(self, drawable)
	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateData)
end

function MyGraph2:hide()
	self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateData)
	ui.Meter.hide(self)
end

function MyGraph2:updateData(msg)
	local p = self.Params
	local c = self.Curves[1]
	local p1, p2, p3 = p[1], p[2], p[3]
	for i = 1, self.NumSamples do
		c[i] = (sintab[p1%256+1] + sintab[p2%256+1] + sintab[p3%256+1]) / 3
		p1 = p1 + 21
		p2 = p2 + 27
		p3 = p3 + 10
	end
	p[1] = p[1] + 5
	p[2] = p[2] + 6
	p[3] = p[3] + 7
	p[1] = p[1] % 256
	p[2] = p[2] % 256
	p[3] = p[3] % 256
	self.RedrawGraph = true
	self:setFlags(ui.FL_CHANGED)
	return msg
end

function MyGraph2:drawGraph()
	local g = self.GraphRect
	local w = g[3] - g[1] + 1
	local h = g[4] - g[2] + 1
	local d = self.Window.Drawable
	local y0 = g[2]
	for i = 1, 5 do
		local y = y0 + (i - 1) * h / 4
		d:drawLine(g[1], floor(y), g[3], floor(y), "user4")
	end
	ui.Meter.drawGraph(self)
end

-------------------------------------------------------------------------------
--	Application:
-------------------------------------------------------------------------------

print "This example can additionally visualize sets of 16bit numbers coming"
print "in via stdin, by dispatching lines to messages of the type MSG_USER."
print "To see the extra curve, run this example as follows:"
print "$ bin/gendata.lua | bin/meter.lua"

ui.Application:new
{
	AuthorStyleSheets = "industrial",
	Display = ui.Display:new
	{
		Style = [[
			rgb-user1: #003300;
			rgb-user3: #cccc55;
			rgb-user2: #ffcc77; 
			rgb-user4: #448822;
		]]
	},
	Children =
	{
		ui.Window:new
		{
			Title = "Graph",
			Orientation = "vertical",
			HideOnEscape = true,
			Children =
			{
				ui.Text:new { Text = "Graph" },
				MyGraph1:new { 
					NumSamples = 256,
					Curves = { sintab, { } },
					GraphBGColor = "user1",
					GraphColor = "user2",
					GraphColor2 = "user3"
				},
				MyGraph2:new { 
					NumSamples = 128,
					GraphColor = "user3"
				},
			}
		}
	}
}:run()
