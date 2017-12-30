#!/usr/bin/env lua

local ui = require "tek.ui"
local List = require "tek.class.list"
local Button = ui.Button
local Group = ui.Group
local Text = ui.Text

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Title = L.ALIGNMENT_TITLE,
	Id = "alignment-window",
	Orientation = "vertical",
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Orientation = "vertical",
			Width = "free",
			Legend = L.ALIGNMENT_HORIZONTAL,
			Children =
			{
				Button:new
				{
					Text = L.ALIGNMENT_LEFT,
					Width = "auto",
					MaxHeight = "none",
					HAlign = "left"
				},
				Button:new
				{
					Text = L.ALIGNMENT_CENTER,
					Width = "auto",
					MaxHeight = "none",
					HAlign = "center"
				},
				Group:new
				{
					Legend = L.ALIGNMENT_GROUP,
					Width = "auto",
					MaxHeight = "none",
					HAlign = "right",
					Children =
					{
						Button:new
						{
							Text = L.ALIGNMENT_RIGHT,
							MaxHeight = "none",
						}
					}
				}
			}
		},
		Group:new
		{
			Height = "free",
			Legend = L.ALIGNMENT_VERTICAL,
			Children =
			{
				Button:new
				{
					Text = L.ALIGNMENT_TOP,
					VAlign = "top"
				},
				Button:new
				{
					Text = L.ALIGNMENT_CENTER,
					VAlign = "center"
				},
				Group:new
				{
					Legend = L.ALIGNMENT_GROUP,
					VAlign = "bottom",
					Children =
					{
						Button:new
						{
							Text = L.ALIGNMENT_BOTTOM,
						}
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
		Name = L.ALIGNMENT_BUTTON,
		Description = L.ALIGNMENT_DESCRIPTION
	}
end
