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
	Orientation = "vertical",
	Id = "borders-window",
	Title = L.BORDER_TITLE,
	Status = "hide",
	HideOnEscape = true,
	Width = "fill",
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Legend = L.BORDER_STYLES,
			Children =
			{
				SameSize = true,
				Columns = 5,
				Group:new
				{
					Legend = L.BORDER_SOLID,
					Orientation = "vertical",
					Children =
					{
						Text:new
						{
							Style = "border-style:solid; border-width: 2;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:solid; border-rim-width: 1; border-focus-width: 1; border-width: 4;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:solid; border-rim-width: 1; border-focus-width: 1; border-width: 6;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:solid; border-rim-width: 1; border-focus-width: 1; border-width: 8;",
							Mode = "button",
						},
					}
				},
				Group:new
				{
					Legend = L.BORDER_INSET,
					Orientation = "vertical",
					Children =
					{
						Text:new
						{
							Style = "border-style:inset; border-rim-width: 1; border-focus-width: 1; border-width: 2;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:inset; border-rim-width: 1; border-focus-width: 1; border-width: 4;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:inset; border-rim-width: 1; border-focus-width: 1; border-width: 6;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:inset; border-rim-width: 1; border-focus-width: 1; border-width: 8;",
							Mode = "button",
						},
					}
				},
				Group:new
				{
					Legend = L.BORDER_OUTSET,
					Orientation = "vertical",
					Children =
					{
						Text:new
						{
							Style = "border-style:outset; border-rim-width: 1; border-focus-width: 1; border-width: 2;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:outset; border-rim-width: 1; border-focus-width: 1; border-width: 4;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:outset; border-rim-width: 1; border-focus-width: 1; border-width: 6;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:outset; border-rim-width: 1; border-focus-width: 1; border-width: 8;",
							Mode = "button",
						},
					}
				},
				Group:new
				{
					Legend = L.BORDER_GROOVE,
					Orientation = "vertical",
					Children =
					{
						Text:new
						{
							Style = "border-style:groove; border-rim-width: 1; border-focus-width: 1; border-width: 2;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:groove; border-rim-width: 1; border-focus-width: 1; border-width: 4;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:groove; border-rim-width: 1; border-focus-width: 1; border-width: 6;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:groove; border-rim-width: 1; border-focus-width: 1; border-width: 8;",
							Mode = "button",
						},
					}
				},
				Group:new
				{
					Legend = L.BORDER_RIDGE,
					Orientation = "vertical",
					Children =
					{
						Text:new
						{
							Style = "border-style:ridge; border-rim-width: 1; border-focus-width: 1; border-width: 2;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:ridge; border-rim-width: 1; border-focus-width: 1; border-width: 4;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:ridge; border-rim-width: 1; border-focus-width: 1; border-width: 6;",
							Mode = "button",
						},
						Text:new
						{
							Style = "border-style:ridge; border-rim-width: 1; border-focus-width: 1; border-width: 8;",
							Mode = "button",
						},
					}
				}
			}
		},
		Group:new
		{
			Legend = L.BORDER_SUB,
			Columns = 6,
			Children =
			{
				ui.Text:new
				{
					Class = "caption",
				},
				ui.Text:new
				{
					Class = "caption",
					Text = "0",
				},
				ui.Text:new
				{
					Class = "caption",
					Text = "1",
				},
				ui.Text:new
				{
					Class = "caption",
					Text = "2",
				},
				ui.Text:new
				{
					Class = "caption",
					Text = "3",
				},
				ui.Text:new
				{
					Class = "caption",
					Text = "5",
				},

				ui.Text:new
				{
					Class = "caption",
					Text = L.BORDER_MAIN,
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 2; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 3; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 4; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 5; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 7; border-rim-width: 1; border-focus-width: 1",
				},

				ui.Text:new
				{
					Class = "caption",
					Text = L.BORDER_RIM,
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 2; border-rim-width: 0; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 3; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 4; border-rim-width: 2; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 5; border-rim-width: 3; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 7; border-rim-width: 5; border-focus-width: 1",
				},

				ui.Text:new
				{
					Class = "caption",
					Text = L.BORDER_FOCUS,
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 2; border-rim-width: 1; border-focus-width: 0",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 3; border-rim-width: 1; border-focus-width: 1",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 4; border-rim-width: 1; border-focus-width: 2",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 5; border-rim-width: 1; border-focus-width: 3",
				},
				ui.Text:new
				{
					Class = "button", Mode = "button", Height = "fill",
					Style = "border-width: 7; border-rim-width: 1; border-focus-width: 5",
				},
			}
		},
		Group:new
		{
			Legend = L.BORDER_INDIVIDUAL_STYLES,
			Children =
			{
				ui.Button:new
				{
					Legend = L.BORDER_CAPTION,
					Text = L.BORDER_CAPTION,
				},
				ui.Button:new
				{
					Style = "border-color: border-focus; border-rim-width: 1; border-rim-color:bright; border-focus-color: dark; border-width: 6 12 6 12;",
					Text = L.BORDER_INDIVIDUAL_STYLE,
					Height = "fill",
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
		Name = L.BORDER_BUTTON,
		Description = L.BORDER_DESCRIPTION
	}
end
