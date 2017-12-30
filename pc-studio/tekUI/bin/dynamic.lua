#!/usr/bin/env lua
local ui = require "tek.ui"
ui.Application:new
{
	ApplicationId = "tekui-demo",
	ProgramName = "Dynamic",
	Domain = "schulze-mueller.de",
	Author = "Timm S. Müller";
	Children =
	{
		ui.Window:new
		{
			HideOnEscape = true,
			Orientation = "vertical",
			Children =
			{
				ui.Group:new
				{
					Legend = "Self Modification",
					Width = "fill",
					Children =
					{
						ui.Button:new
						{
							Id = "add-button",
							NumButtons = 0,
							Width = "auto",
							Text = "Add",
							onClick = function(self)
								if self.NumButtons < 10 then
									self:getParent():addMember(ui.Button:new {
										Width = "auto",
										Text = "Remove",
										onClick = function(self)
											local add = self:getById("add-button")
											add.NumButtons = add.NumButtons - 1
											self:getParent():remMember(self)
										end			
									})
									self.NumButtons = self.NumButtons + 1
								end
							end
						}
					}
				},
				ui.Group:new
				{
					Legend = "Dynamic Weight",
					Orientation = "vertical",
					Children =
					{
						ui.Group:new
						{
							Children =
							{
								ui.Slider:new
								{
									Child = ui.Text:new
									{
										Id = "dynweight-knob",
										Class = "knob button",
									},
									Id = "slider-2",
									Min = 0,
									Max = 0x10000,
									Width = "free",
									Default = 0x8000,
									Increment = 0x400,
									show = function(self, drawable)
										ui.Slider.show(self, drawable)
										self:onSetValue()
									end,
									onSetValue = function(self)
										ui.Slider.onSetValue(self)
										local val = self.Value
										local e = self:getById("slider-weight-1")
										e:setValue("Weight", val)
										e:getParent():rethinkLayout()
										e:setValue("Text", ("$%05x"):format(val))
										val = math.floor(val)
										e:getById("dynweight-knob"):setValue("Text", val)
									end
								},
								ui.Button:new
								{
									Text = "Reset",
									VAlign = "center",
									Width = "auto",
									onClick = function(self)
										self:getById("slider-2"):reset()
									end
								}
							}
						},
						ui.Group:new
						{
							Children =
							{
								ui.Text:new 
								{
									Id="slider-weight-1", 
									Style = "font: ui-huge",
									Height = "fill",
								},
								ui.Frame:new { Height = "fill" }
							}
						}
					}
				},
				ui.Group:new
				{
					Width = "free",
					Legend = "Dynamic Layout",
					Children =
					{
						ui.Slider:new
						{
							Child = ui.Text:new
							{
								Id = "dynlayout-knob",
								Class = "knob button",
							},
							Height = "fill",
							Min = 0,
							Max = 100000,
							Increment = 100,
							show = function(self, drawable)
								ui.Slider.show(self, drawable)
								self:onSetValue()
							end,
							onSetValue = function(self)
								ui.Slider.onSetValue(self)
								local val = self.Value
								local text = ("%d"):format(val)
								if val == 100000 then
									text = text .. "\nMaximum"
								end
								self:getById("text-field-1"):setValue("Text", text)
								self:getById("dynlayout-knob"):setValue("Text", math.floor(val))
							end
						},
						ui.Text:new
						{
							Style = "font: ui-huge",
							Id = "text-field-1",
							Width = "auto",
						}
					}
				}
			}
		}
	}
}:run()
