#!/usr/bin/env lua

local ui = require "tek.ui"

local Widget = ui.Widget
local Group = ui.Group
local CheckMark = ui.CheckMark
local RadioButton = ui.RadioButton

local floor = math.floor
local Region = require "tek.lib.region"

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

local RadioImage1 = ui.getStockImage("radiobutton")
local RadioImage2 = ui.getStockImage("radiobutton", 2)
local BitMapImage1 = ui.loadImage(ui.ProgDir .. "/graphics/world.ppm")
local BitMapImage2 = ui.loadImage(ui.ProgDir .. "/graphics/locale_alpha.png")
	or ui.loadImage(ui.ProgDir .. "/graphics/locale.ppm")
local FileImage = ui.getStockImage("file")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Status = "hide",
	Id = "graphics-window",
	Title = L.GRAPHICS_TITLE,
	Width = "fill",
	Height = "fill",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Orientation = "vertical",
			Legend = L.GRAPHICS_CHECKMARKS,
			Style = "height: fill",
			Children =
			{
				CheckMark:new { Text = "xx-small", Style = "font: ui-xx-small" },
				CheckMark:new { Text = "x-small", Style = "font: ui-x-small" },
				CheckMark:new { Text = "small", Style = "font: ui-small" },
				CheckMark:new { Text = "main", Style = "font: ui-main", 
					Selected = true },
				CheckMark:new { Text = "large", Style = "font: ui-large" },
				CheckMark:new { Text = "x-large", Style = "font: ui-x-large" },
				CheckMark:new { Text = "xx-large", Style = "font: ui-xx-large" },
			}
		},
		Group:new
		{
			Orientation = "vertical",
			Legend = L.GRAPHICS_RADIOBUTTONS,
			Style = "height: fill",
			Children =
			{
				RadioButton:new { Text = "xx-small", Style = "font: ui-xx-small" },
				RadioButton:new { Text = "x-small", Style = "font: ui-x-small" },
				RadioButton:new { Text = "small", Style = "font: ui-small" },
				RadioButton:new { Text = "main", Style = "font: ui-main",
					Selected = true },
				RadioButton:new { Text = "large", Style = "font: ui-large" },
				RadioButton:new { Text = "x-large", Style = "font: ui-x-large" },
				RadioButton:new { Text = "xx-large", Style = "font: ui-xx-large" },
			}
		},
		Group:new
		{
			Orientation = "vertical",
			Legend = L.GRAPHICS_BITMAPS,
			Style = "height: fill",
			Children =
			{
				ui.ImageWidget:new
				{
					Mode = "inert",
					Class = "button",
					Image = BitMapImage2,
					Style = "background-color: gradient(0,0,#238,0,100,#aaf); padding: 4",
				},
				ui.ImageWidget:new
				{
					Style = "background-color: #fff",
					Image = BitMapImage1
				}
			}
		},
		Group:new
		{
			Orientation = "vertical",
			Legend = L.GRAPHICS_BACKGROUND,
			Style = "height: fill",
			Children =
			{
				ui.Group:new
				{
					Style = [[
						height: fill; 
						background-color: url(bin/graphics/world.ppm);
					]],
					Children =
					{
						ui.ImageWidget:new 
						{
							Height = "fill",
							Mode = "button",
							Style = [[
								background-color: transparent; 
								margin: 10;
								padding: 4;
								border-width: 10;
								border-focus-width: 6;
								min-width: 60;
							]],
							Image = FileImage
						},
					}
				}
			}
		}
	}
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
		Name = L.GRAPHICS_TITLE,
		Description = L.GRAPHICS_DESCRIPTION,
	}
end
