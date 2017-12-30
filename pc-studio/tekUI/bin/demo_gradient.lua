#!/usr/bin/env lua

local ui = require "tek.ui"
local Group = ui.Group
local Text = ui.Text
local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Style = [[
		background-color:gradient(100,100,shine,300,200,shadow);
		padding:20;
	]],
	Orientation = "vertical",
	Id = "gradient-window",
	Title = L.GRADIENT_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Width = "fill",
	Legend = L.GRADIENT_LEGEND_WINDOW_GROUP,
	Children =
	{
		Group:new
		{
			Style= [[
				background-color:transparent; 
				padding:20;
				margin: 20;
			]],
			Legend = L.GRADIENT_LEGEND_GROUP_BORDER,
			Children =
			{
				ui.Button:new
				{
					Legend = L.GRADIENT_LEGEND_BUTTON_BORDER,
					Style= [[
						padding:20;
						border-width: 7;
						border-focus-width: 3;
						background-color: transparent;
					]],
					Text = L.GRADIENT_BUTTON,
					Height = "free",
				},
				ui.Button:new
				{
					Legend = L.GRADIENT_LEGEND_BUTTON_BORDER,
					Style= [[
						padding:20;
						border-width: 7;
						border-focus-width: 3;
						background-color: transparent;
					]],
					Text = L.GRADIENT_BUTTON,
					Height = "free",
				},
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
		Name = L.GRADIENT_TITLE,
		Description = L.GRADIENT_DESCRIPTION,
	}
end
