#!/usr/bin/env lua

local ui = require "tek.ui"
local Group = ui.Group
local Text = ui.Text

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Helper classes:
-------------------------------------------------------------------------------

local VerboseCheckMark = ui.CheckMark:newClass { _NAME = "_vbcheckmark" }

function VerboseCheckMark:onSelect()
	ui.CheckMark.onSelect(self)
	local tw = self:getById("text-window")
	local text = self.Selected and L.SELECTED or L.REVOKED
	text = text .. ": " .. self.Text:gsub("_", "")
	tw:appendLine(text, true)
end

local VerboseRadioButton = ui.RadioButton:newClass { _NAME = "_vbradiobutton" }

function VerboseRadioButton:onSelect()
	ui.RadioButton.onSelect(self)
	if self.Selected then
		local tw = self:getById("text-window")
		local text = L.SELECTED .. ": " .. self.Text:gsub("_", "")
		tw:appendLine(text, true)
	end
end

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Id = "choices-window",
	Title = L.CHOICES_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Width = "fill",
			Legend = L.CHOICES_ORDER_BEVERAGES,
			Children =
			{
				ui.Group:new {
					Orientation = "vertical",
					Children = 
					{
						VerboseRadioButton:new
						{
							Text = L.CHOICES_COFFEE,
							Id = "drink-coffee",
							Selected = true,
							onSelect = function(self)
								VerboseRadioButton.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", true)
									self:getById("drink-hot"):setValue("Disabled", false)
									self:getById("drink-ice"):setValue("Disabled", false)
									self:getById("drink-straw"):setValue("Disabled", false)
									self:getById("drink-shaken"):setValue("Disabled", false)
									self:getById("drink-stirred"):setValue("Selected", false)
									self:getById("drink-stirred"):setValue("Disabled", false)
								end
							end,
						},
						VerboseRadioButton:new
						{
							Text = L.CHOICES_JUICE,
							Id = "drink-juice",
							onSelect = function(self)
								VerboseRadioButton.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-hot"):setValue("Disabled", true)
									self:getById("drink-ice"):setValue("Disabled", false)
									self:getById("drink-straw"):setValue("Disabled", false)
									self:getById("drink-shaken"):setValue("Disabled", false)
									self:getById("drink-stirred"):setValue("Disabled", false)
								end
							end,
						},
						VerboseRadioButton:new
						{
							Text = L.CHOICES_MANGO_LASSI,
							Id = "drink-lassi",
							onSelect = function(self)
								VerboseRadioButton.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-hot"):setValue("Disabled", true)
									self:getById("drink-ice"):setValue("Disabled", false)
									self:getById("drink-straw"):setValue("Disabled", false)
									self:getById("drink-straw"):setValue("Selected", true)
									self:getById("drink-shaken"):setValue("Selected", false)
									self:getById("drink-shaken"):setValue("Disabled", true)
									self:getById("drink-stirred"):setValue("Disabled", true)
									self:getById("drink-stirred"):setValue("Selected", true)
								end
							end,
						},
						VerboseRadioButton:new
						{
							Text = L.CHOICES_BEER,
							Id = "drink-beer",
							onSelect = function(self)
								VerboseRadioButton.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-hot"):setValue("Disabled", true)
									self:getById("drink-ice"):setValue("Selected", false)
									self:getById("drink-ice"):setValue("Disabled", true)
									self:getById("drink-straw"):setValue("Selected", false)
									self:getById("drink-straw"):setValue("Disabled", true)
									self:getById("drink-shaken"):setValue("Selected", false)
									self:getById("drink-shaken"):setValue("Disabled", true)
									self:getById("drink-stirred"):setValue("Selected", false)
									self:getById("drink-stirred"):setValue("Disabled", true)
								end
							end,
						},
						VerboseRadioButton:new
						{
							Text = L.CHOICES_WHISKY,
							Id = "drink-whisky",
							onSelect = function(self)
								VerboseRadioButton.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-hot"):setValue("Disabled", true)
									self:getById("drink-ice"):setValue("Disabled", false)
									self:getById("drink-straw"):setValue("Disabled", false)
									self:getById("drink-shaken"):setValue("Disabled", false)
									self:getById("drink-stirred"):setValue("Disabled", false)
								end
							end,
						}
						
					}
				},
				ui.Group:new {
					Orientation = "vertical",
					Children = {
						
						VerboseCheckMark:new
						{
							Text = L.CHOICES_HOT,
							Id = "drink-hot",
							Selected = true,
							onSelect = function(self)
								VerboseCheckMark.onSelect(self)
								if self.Selected then
									self:getById("drink-ice"):setValue("Selected", false)
									self:getById("drink-straw"):setValue("Selected", false)
									self:getById("drink-shaken"):setValue("Selected", false)
								end
							end,
						},
						VerboseCheckMark:new
						{
							Text = L.CHOICES_WITH_ICE,
							Id = "drink-ice",
							onSelect = function(self)
								VerboseCheckMark.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-straw"):setValue("Selected", true)
								end
							end,
						},
						VerboseCheckMark:new
						{
							Text = L.CHOICES_STIRRED,
							Id = "drink-stirred",
							onSelect = function(self)
								VerboseCheckMark.onSelect(self)
								if self.Selected then
									self:getById("drink-shaken"):setValue("Selected", false)
								end
							end,
						},
						VerboseCheckMark:new
						{
							Text = L.CHOICES_SHAKEN,
							Id = "drink-shaken",
							onSelect = function(self)
								VerboseCheckMark.onSelect(self)
								if self.Selected then
									self:getById("drink-stirred"):setValue("Selected", false)
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-ice"):setValue("Selected", true)
								end
							end,
						},
						VerboseCheckMark:new
						{
							Text = L.CHOICES_DRINKING_STRAW,
							Id = "drink-straw",
							onSelect = function(self)
								VerboseCheckMark.onSelect(self)
								if self.Selected then
									self:getById("drink-hot"):setValue("Selected", false)
									self:getById("drink-ice"):setValue("Selected", true)
								end
							end,
						}
						
					}
				}
			}
				
		},
		ui.ScrollGroup:new
		{
			Legend = L.OUTPUT,
			VSliderMode = "auto",
			Child = ui.Canvas:new
			{
				AutoWidth = true,
				Child = ui.FloatText:new
				{
					Id = "text-window",
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
		Name = L.CHOICES_TITLE,
		Description = L.CHOICES_DESCRIPTION,
	}
end
