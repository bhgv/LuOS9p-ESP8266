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
	Id = "editor-window",
	Title = L.INPUT_TITLE,
	Status = "hide",
	HideOnEscape = true,
	Height = "free",
	Columns = 2,
	SizeButton = true,
	Children =
	{
		ui.Text:new
		{
			Class = "caption",
			Width = "auto",
			Text = L.INPUT_FIELD,
		},
		ui.Input:new 
		{
			Text = L.INPUT_FIELD_TEXT
		},
				
		ui.Text:new
		{
			Class = "caption",
			Width = "auto",
			Text = L.INPUT_FIELD_MAX_LENGTH,
		},
		ui.Input:new 
		{
			Text = L.INPUT_FIELD_TEXT,
			MaxLength = 20,
		},
		
		ui.Text:new
		{
			Class = "caption",
			Width = "auto",
			Text = L.INPUT_FIELD_PASSWORD,
		},
		ui.Input:new 
		{
			Text = "Swordfish",
			MaxLength = 20,
			PasswordChar = "•",
		},
		
		ui.Text:new
		{
			Class = "caption",
			Width = "auto",
			Text = L.INPUT_FIELD_FIXED_WIDTH,
		},
		ui.Input:new 
		{
			Text = L.INPUT_FIELD,
			Style = "font:ui-fixed",
		},
		
		ui.Text:new
		{
			Class = "caption",
			Width = "auto",
			Text = L.INPUT_FIELD_MULTILINE,
		},
		ui.Input:new 
		{
			Text = L.INPUT_FIELD_MULTILINE_TEXT,
			MultiLine = true,
			Height = "free",
		},
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
		Name = L.INPUT_TITLE,
		Description = L.INPUT_DESCRIPTION,
	}
end
