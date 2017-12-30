#!/usr/bin/env lua

local ui = require "tek.ui"
local Group = ui.Group
local Text = ui.Text

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	ScrollText:
-------------------------------------------------------------------------------

local ScrollText = ui.FloatText:newClass { _NAME = "_scrolltext" }

function ScrollText.new(class, self)
	self.YPos = 0
	self.YDelta = 7
	self.Delay = 100
	self.SliderElement = self.SliderElement or false
	self.LineBuffer = { }
	self.BufLines = self.BufLines or 16
	for i = 1, self.BufLines do
		self.LineBuffer[i] = ""
	end
	self.Slides = self.Slides or { { } }
	self.CurrentSlide = 1
	self.UpDown = 1
	self.NextDelay = 0
	self = ui.FloatText.new(class, self)
	self:nextSlide()
	self.Delay = 0
	return self
end

function ScrollText:nextSlide()
	local s = self.Slides[self.CurrentSlide]
	local bufh = self.BufLines
	local half = self.UpDown % 2 * bufh / 2 + 1
	for i = half, half + bufh / 2 - 1 do
		self.LineBuffer[i] = ""
	end
	local lnr = half + bufh / 4 - math.floor((#s+1) / 2)
	local j = 1
	local tlen = 0
	for i = lnr, lnr + #s - 1 do
		local text = s[j]
		tlen = tlen + text:len()
		self.LineBuffer[i] = " " .. text
		j = j + 1
	end
	self.NextDelay = 200 + tlen * 2
	self:setValue("Text", table.concat(self.LineBuffer, "\n"))
	self.CurrentSlide = self.CurrentSlide + 1
	if self.CurrentSlide > #self.Slides then
		self.CurrentSlide = 1
	end
	self.UpDown = self.UpDown % 2 + 1
end

function ScrollText:show()
	ui.FloatText.show(self)
	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
end

function ScrollText:hide()
	self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
	ui.FloatText.hide(self)
end

function ScrollText:updateInterval(msg)
	if self.SliderElement then
		local ymax = self.SliderElement.VSliderGroup.Slider.Max
		if self.Delay == 0 then
			local ypos = self.YPos
			ypos = ypos + self.YDelta
			if ypos < 0 or ypos > ymax then
				self.YDelta = -self.YDelta
				ypos = math.max(ypos, 0)
				ypos = math.min(ypos, ymax)
				self.Delay = self.NextDelay
				self:nextSlide()
			end
			self.YPos = ypos
			ypos = (math.cos((ymax - ypos) * math.pi / ymax) / 2 + 0.5) * ymax
			self.SliderElement.Child:setValue("CanvasTop", math.floor(ypos))
		end
		if self.Delay > 0 then
			self.Delay = self.Delay - 1
		end
	end
	return msg
end

-------------------------------------------------------------------------------

local Presenter = ScrollText:new
{
	Style = "font: ui-huge:40; background-color: dark; color: bright",
-- 	Class = "text-slide",
	Preformatted = true,
	Slides =
	{
		{
			"» Welcome to tekUI, the",
			"scriptable GUI toolkit",
			"for embedded applications",
			"and kiosk systems.",
		},
		{
			"» This demonstration",
			"is written in Lua, tekUI's",
			"scripting language.",
		},
		{
			"» The GUI runs managed",
			"in a virtual machine and",
			"cannot crash the device",
			"or application.",
		},
		{
			"» GUI code is compact,",
			"easy to learn, program,",
			"maintain and extend.",
		},
	}
}

local SlideScroll = ui.ScrollGroup:new
{
	Weight = 0x10000,
	VSliderMode = "on",
	Child = ui.Canvas:new
	{
		AutoWidth = true,
		Child = Presenter
	}
}

SlideScroll.VSliderGroup.Slider:addNotify("Active", true, { ui.NOTIFY_SELF, ui.NOTIFY_FUNCTION, function(self, active)
	Presenter.Delay = Presenter.NextDelay
end, ui.NOTIFY_VALUE })
SlideScroll.VSliderGroup.Slider:addNotify("Hold", true, { ui.NOTIFY_SELF, ui.NOTIFY_FUNCTION, function(self, active)
	Presenter.Delay = Presenter.NextDelay
end, ui.NOTIFY_VALUE })
SlideScroll.VSliderGroup.Slider:addNotify("Min", ui.NOTIFY_ALWAYS, { ui.NOTIFY_SELF, ui.NOTIFY_FUNCTION, function(self, active)
	Presenter.Delay = Presenter.NextDelay
end, ui.NOTIFY_VALUE })

SlideScroll.Child.Child.SliderElement = SlideScroll

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Id = "presentation-window",
	Style = "Width: 600; Height: 400",
	Title = L.PRESENTATION_TITLE,
	Orientation = "vertical",
	Status = "hide",
	HideOnEscape = true,
	Children = { SlideScroll }
}

-------------------------------------------------------------------------------
--	Started stand-alone or as part of the demo?
-------------------------------------------------------------------------------

if ui.ProgName:match("^demo_") then
	local app = ui.Application:new()
	ui.Application.connect(window)
	app:addMember(window)
	window:setValue("Status", "show")
	app:run()
else
	return
	{
		Window = window,
		Name = L.PRESENTATION_TITLE,
		Description = L.PRESENTATION_DESCRIPTION,
	}
end
