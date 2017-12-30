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
	Id = "buttons-window",
	Title = L.BUTTONS_TITLE,
	Orientation = "vertical",
	Status = "hide",
	HideOnEscape = true,
	Width = "fill",
	Height = "fill",
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Children =
			{
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_CAPTION_STYLE,
					Height = "fill",
					Children =
					{
						Text:new { Class = "caption", Text = "XX-Small",
							Style = "font: ui-xx-small" },
						Text:new { Class = "caption", Text = "X-Small",
							Style = "font: ui-x-small" },
						Text:new { Class = "caption", Text = "Small",
							Style = "font: ui-small" },
						Text:new { Class = "caption", Text = "Main",
							Style = "font: ui-main" },
						Text:new { Class = "caption", Text = "Large",
							Style = "font: ui-large" },
						Text:new { Class = "caption", Text = "XL",
							Style = "font: ui-x-large" },
						Text:new { Class = "caption", Text = "XXL",
							Style = "font: ui-xx-large" },
						Text:new { Class = "caption", Text = "Fixed",
							Style = "font: ui-fixed" },
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_NORMAL_STYLE,
					Height = "fill",
					Children =
					{
						Text:new { Text = "XX-Small",
							Style = "font: ui-xx-small" },
						Text:new { Text = "X-Small",
							Style = "font: ui-x-small" },
						Text:new { Text = "Main",
							Style = "font: ui-main" },
						Text:new { Text = "Large",
							Style = "font: ui-large" },
						Text:new { Text = "XL",
							Style = "font: ui-x-large" },
						Text:new { Text = "XXL",
							Style = "font: ui-xx-large" },
						Text:new { Text = "Fixed",
							Style = "font: ui-fixed" },
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_BUTTON,
					Height = "fill",
					Children =
					{
						Button:new { Text = "XX-Small", Style = "font: ui-xx-small" },
						Button:new { Text = "X-Small", Style = "font: ui-x-small" },
						Button:new { Text = "Small", Style = "font: ui-small" },
						Button:new { Text = "Main", Style = "font: ui-main" },
						Button:new { Text = "Large", Style = "font: ui-large" },
						Button:new { Text = "XL", Style = "font: ui-x-large" },
						Button:new { Text = "XXL", Style = "font: ui-xx-large" },
						Button:new { Text = "Fixed", Style = "font: ui-fixed" },
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_COLORS,
					Height = "fill",
					Children =
					{
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: dark; color: bright" },
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: shadow; color: shine" },
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: half-shadow; color: shine" },
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: half-shine; color: dark" },
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: bright; color: dark" },
						Button:new { Text = L.BUTTONS_BUTTON, Style = "background-color: #aa0000; color: #ffff00" },
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_TEXT_ALIGNMENTS,
-- 					Height = "fill",
					SameSize = "height",
					Children =
					{
						Button:new { Text = L.BUTTONS_TOP_LEFT, Style = "text-align: left; vertical-align: top; height: free" },
						Button:new { Text = L.BUTTONS_CENTER, Style = "text-align: center; vertical-align: center; height: free" },
						Button:new { Text = L.BUTTONS_RIGHT_BOTTOM, Style = "text-align: right; vertical-align: bottom; height: free" },
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Legend = L.BUTTONS_TEXT_STYLES,
					Height = "fill",
					SameSize = "height",
					Children =
					{
						Button:new { Text = "Default", Style = "font: ui-main" },
						Button:new { Text = "Italic", Style = "font: ui-main/i" },
						Button:new { Text = "Bold", Style = "font: ui-main/b" },
						Button:new { Text = "BoldItalic", Style = "font: ui-main/bi" },
					}
				}
			}
		},
		Group:new
		{
			Legend = L.WIDGET_MODES,
			SameSize = true,
			Children =
			{
				Button:new
				{
					Text = "inert", Mode = "inert",
				},
				Button:new
				{
					Text = "toggle", Mode = "toggle",
				},
				Button:new
				{
					Text = "button", Mode = "button",
				},
				Button:new
				{
					Id = "buttons-touch",
					Font = "ui-fixed", Text = "touch", Mode = "touch",
					onSelect = function(self)
						Button.onSelect(self)
						self:getById("buttons-unselect"):
							setValue("Disabled", not self.Selected)
					end
				},
				Button:new
				{
					Id = "buttons-unselect",
					Disabled = true,
					Text = "« " .. L.WIDGET_UNSELECT,
					onClick = function(self)
						self:getById("buttons-touch"):
							setValue("Selected", false)
					end
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
		Name = L.BUTTONS_TITLE,
		Description = L.BUTTONS_DESCRIPTION,
	}
end
