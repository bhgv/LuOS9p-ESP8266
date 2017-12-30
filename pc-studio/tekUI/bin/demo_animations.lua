#!/usr/bin/env lua

local ui = require "tek.ui"
local Group = ui.Group
local Slider = ui.Slider
local Text = ui.Text
local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Helper class:
-------------------------------------------------------------------------------

local Coefficient = Group:newClass { _NAME = "_coefficient-group" }

function Coefficient.new(class, self)
	local group = self -- for building a closure with the group
	self.Children =
	{
		Text:new 
		{ 
			Class = "caption", 
			Text = self.Key1,
			Width = "auto",
		},
		Slider:new
		{
			Min = 0,
			Max = 31,
			Value = self.Value1,
			Increment = 3,
			Step = 1,
			onSetValue = function(self)
				Slider.onSetValue(self)
				local p = self:getById("the-plasma")
				p:setValue(group.Key1, self.Value)
			end,
		},
		Text:new 
		{ 
			Class = "caption", 
			Text = self.Key2, 
			Width = "auto",
		},
		Slider:new
		{
			Min = -16,
			Max = 15,
			Value = self.Value2,
			Increment = 3,
			Step = 1,
			onSetValue = function(self)
				Slider.onSetValue(self)
				local p = self:getById("the-plasma")
				p:setValue(group.Key2, self.Value)
			end,
		},
	}
	return Group.new(class, self)
end

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Orientation = "vertical",
	Width = 400,
	MinWidth = 0,
	MaxWidth = "none", 
	MaxHeight = "none",
	Id = "anims-window",
	Title = L.ANIMATIONS_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		ui.PageGroup:new
		{
			PageCaptions = { "_Tunnel", "_Boing", "_Plasma" },
			Children =
			{
				Group:new
				{
					Children =
					{
						Group:new
						{
							Orientation = "vertical",
							Children =
							{
								ui.Tunnel:new
								{
									Id = "the-tunnel",
								},
								Group:new
								{
									Legend = L.ANIMATIONS_PARAMETERS,
									Columns = 2,
									Children =
									{
										Text:new
										{
											Text = L.ANIMATIONS_SPEED,
											Class = "caption",
											Width = "fill"
										},
										ui.Slider:new
										{
											Min = 1,
											Max = 19,
											Value = 8,
											Range = 20,
											onSetValue = function(self)
												ui.Slider.onSetValue(self)
												self:getById("the-tunnel"):setSpeed(self.Value)
											end
										},
										Text:new
										{
											Text = L.ANIMATIONS_FOCUS,
											Class = "caption",
											Width = "fill"
										},
										ui.Slider:new
										{
											Min = 0x10,
											Max = 0x1ff,
											Value = 0x50,
											Range = 0x200,
											Increment = 20,
											onSetValue = function(self)
												ui.Slider.onSetValue(self)
												self:getById("the-tunnel"):setViewZ(self.Value)
											end
										},
										Text:new
										{
											Text = L.ANIMATIONS_SEGMENTS,
											Class = "caption",
											Width = "fill"
										},
										ui.Slider:new 
										{
											Min = 1,
											Max = 19,
											Value = 6,
											Range = 20,
											onSetValue = function(self)
												ui.Slider.onSetValue(self)
												self:getById("the-tunnel"):setNumSeg(self.Value)
											end
										}
									}
								}
							}
						}
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Children =
					{
						Group:new
						{
							Children =
							{
								Slider:new
								{
									Id = "boing-slider",
									Orientation = "vertical",
									Min = 0,
									Max = 0x10000,
									Range = 0x14000,
									Style = "border-width: 0; background-color: dark; margin: 0";
								},
								ui.Boing:new
								{
									Id = "the-boing",
									Style = "border-width: 0; margin: 0",
									onSetYPos = function(self, ypos)
										local s = self:getById("boing-slider")
										s:setValue("Value", ypos)
										local s = self:getById("boing-slider2")
										s:setValue("Value", ypos)
									end,
								},
								Slider:new
								{
									Id = "boing-slider2",
									Orientation = "vertical",
									Min = 0,
									Max = 0x10000,
									Range = 0x14000,
									Style = "border-width: 0; background-color: dark; margin: 0";
								}
							}
						},
						Group:new
						{
							Children =
							{
								ui.Button:new 
								{
									Text = L.ANIMATIONS_START,
									onClick = function(self)
										self:getById("the-boing").Running = true
									end
								},
								ui.Button:new 
								{
									Text = L.ANIMATIONS_STOP,
									onClick = function(self)
										self:getById("the-boing").Running = false
									end
								}														
							}
						}
					}
				},
				Group:new
				{
					Orientation = "vertical",
					Children =
					{
						ui.Plasma:new { Id = "the-plasma" },
						Group:new
						{
							Width = "fill",
							Orientation = "vertical",
							Legend = L.COEFFICIENTS,
							Children =
							{
								Coefficient:new { Key1 = "DeltaX1", Value1 = 12, Key2 = "SpeedX1", Value2 = 7 },
								Coefficient:new { Key1 = "DeltaX2", Value1 = 13, Key2 = "SpeedX2", Value2 = -2 },
								Coefficient:new { Key1 = "DeltaY1", Value1 = 8, Key2 = "SpeedY1", Value2 = 9 },
								Coefficient:new { Key1 = "DeltaY2", Value1 = 11, Key2 = "SpeedY2", Value2 = -4 },
								Coefficient:new { Key1 = "DeltaY3", Value1 = 18, Key2 = "SpeedY3", Value2 = 5 },
							}
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
		Name = L.ANIMATIONS_BUTTON,
		Description = L.ANIMATIONS_DESCRIPTION
	}
end
