#!/usr/bin/env lua

local ui = require "tek.ui"
local Button = ui.Button
local Group = ui.Group
local Text = ui.Text

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Orientation = "vertical",
	Id = "layout-window",
	Title = L.LAYOUT_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Legend = L.LAYOUT_RELATIVE_SIZES,
			Children =
			{
				Button:new { Text = "1" },
				ui.Spacer:new { },
				Button:new { Text = "12" },
				ui.Spacer:new { },
				Button:new { Text = "123" },
				ui.Spacer:new { },
				Button:new { Text = "1234" },
			},
		},
		Group:new
		{
			SameSize = "width",
			Legend = L.LAYOUT_SAME_SIZES,
			Children =
			{
				Button:new { Text = "1" },
				ui.Spacer:new { },
				Button:new { Text = "12" },
				ui.Spacer:new { },
				Button:new { Text = "123" },
				ui.Spacer:new { },
				Button:new { Text = "1234" },
			}
		},
		Group:new
		{
			Legend = L.LAYOUT_BALANCING_GROUP,
			Orientation = "vertical",
			Children =
			{
				Group:new
				{
					Children =
					{
						Text:new { Text = "free", Style = "height: fill" },
						ui.Handle:new { },
						Text:new { Text = "free", Style = "height: fill" },
						ui.Handle:new { },
						Text:new { Text = "free", Style = "height: fill" },
					}
				},
				ui.Handle:new { },
				Group:new
				{
					Children =
					{
						Text:new { Text = "free", Style = "height: fill" },
						ui.Handle:new { },
						Text:new { Text = "free", Style = "height: fill" },
						ui.Handle:new { },
						Text:new { Text = "free", Style = "height: fill" },
					}
				},
			}
		},
		Group:new
		{
			Legend = L.LAYOUT_GRID,
			Columns = 3,
			SameSize = "width",
			Children =
			{
				Button:new { Text = "1" },
				Button:new { Text = "12" },
				Button:new { Text = "123" },
				Button:new { Text = "1234" },
				Button:new { Text = "12345" },
				Button:new { Text = "123456" },
			}
		},
		Group:new
		{
			Legend = L.LAYOUT_DIFFERENT_WEIGHTS,
			Children =
			{
				Button:new { Text = "25%", Weight = 0x4000 },
				ui.Spacer:new { },
				Button:new { Text = "25%", Weight = 0x4000 },
				ui.Spacer:new { },
				Button:new { Text = "50%", Weight = 0x8000 },
			}
		},
		Group:new
		{
			Legend = L.LAYOUT_FIXED_VS_FREE,
			Children =
			{
				Text:new { Text = L.LAYOUT_FIX, Height = "fill" },
				Button:new { Text = "25%", Weight = 0x4000 },
				Text:new { Text = L.LAYOUT_FIX, Height = "fill" },
				Button:new { Text = "75%", Weight = 0xc000 },
				Text:new { Text = L.LAYOUT_FIX, Height = "fill" },
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
		Name = L.LAYOUT_TITLE,
		Description = L.LAYOUT_DESCRIPTION,
	}
end
