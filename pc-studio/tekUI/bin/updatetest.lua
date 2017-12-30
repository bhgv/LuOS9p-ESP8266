#!/usr/bin/env lua

local ui = require "tek.ui"
ui.ThemeName = "dark tutorial"
ui.UserStyles = false

ui.Application:new {
	Children = {
		ui.Window:new {
			Index = 0,
			updateInterval = function(self)
				for i = 1, 10 do
					local n = math.random(1, 28)
					local e = self:getById("button"..n)
					e:setValue("Text", tostring(self.Index))
					self.Index = self.Index + 1
					if self.Index == 10000 then
						self.Application:quit()
					end
				end
			end,
			show = function(self)
				ui.Window.show(self)
				self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
			end,
			hide = function(self)
				ui.Window.hide(self)
				self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
			end,
			Width = "auto",
			HideOnEscape = true,
			Columns = 2,
			Children = {
				ui.Group:new {
					Legend = "outer1",
					Columns = 2,
					SameSize = true,
					Children = {
						ui.Button:new { Id = "button1", },
						ui.Button:new { Id = "button2", },
						ui.Button:new { Id = "button3", },
						ui.Group:new {
							Columns = 2,
							SameSize = true,
							Legend = "inner1",
							Children = {
								ui.Button:new { Id = "button13", },
								ui.Button:new { Id = "button14", },
								ui.Button:new { Id = "button15", },
								ui.Button:new { Id = "button16", }
							}
						}
					}
				},
				ui.Group:new {
					Columns = 2,
					SameSize = true,
					Legend = "outer2",
					Children = {
						ui.Button:new { Id = "button4", },
						ui.Button:new { Id = "button5", },
						ui.Button:new { Id = "button6", },
						ui.Group:new {
							Columns = 2,
							SameSize = true,
							Legend = "inner2",
							Children = {
								ui.Button:new { Id = "button17", },
								ui.Button:new { Id = "button18", },
								ui.Button:new { Id = "button19", },
								ui.Button:new { Id = "button20", }
							}
						}
					}
				},
				ui.Group:new {
					Columns = 2,
					SameSize = true,
					Legend = "outer3",
					Children = {
						ui.Button:new { Id = "button7", },
						ui.Button:new { Id = "button8", },
						ui.Button:new { Id = "button9", },
						ui.Group:new {
							Columns = 2,
							SameSize = true,
							Legend = "inner3",
							Children = {
								ui.Button:new { Id = "button21", },
								ui.Button:new { Id = "button22", },
								ui.Button:new { Id = "button23", },
								ui.Button:new { Id = "button24", }
							}
						}
					}
				},
				ui.Group:new {
					Columns = 2,
					SameSize = true,
					Legend = "outer4",
					Children = {
						ui.Button:new { Id = "button10", },
						ui.Button:new { Id = "button11", },
						ui.Button:new { Id = "button12", },
						ui.Group:new {
							Columns = 2,
							SameSize = true,
							Legend = "inner4",
							Children = {
								ui.Button:new { Id = "button25", },
								ui.Button:new { Id = "button26", },
								ui.Button:new { Id = "button27", },
								ui.Button:new { Id = "button28", }
							}
						}
					}
				}
			}
		}
	}
}:run()
