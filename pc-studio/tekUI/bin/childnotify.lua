#!/usr/bin/env lua
local exec = require "tek.lib.exec"

local c = exec.run(function()
	local ui = require "tek.ui"
	ui.Application:new
	{
		handleAbort = function(self)
			self:addCoroutine(function()
				if self:easyRequest(false, "Parent task wants to quit!",
					"Quit too", "Cancel") == 1 then
					self:quit()
				end
			end)
			return false
		end,
		setup = function(self)
			ui.Application.setup(self)
			self:addInputHandler(ui.MSG_SIGNAL, self, self.handleAbort)
		end,
		cleanup = function(self)
			self:remInputHandler(ui.MSG_SIGNAL, self, self.handleAbort)
			ui.Application.cleanup(self)
		end,
		Children =
		{
			ui.Window:new
			{
				HideOnEscape = true,
				Children =
				{
					ui.Text:new 
					{ 
						Text = "GUI in child task",
						Style = "font: ui-huge",
						Width = "free", Height = "free",
					}
				}
			}
		}
	}:run()
end)

local ui = require "tek.ui"
ui.Application:new
{
	handleAbort = function(self)
		self:addCoroutine(function()
			if self:easyRequest(false, "Child task has quit!",
				"Quit too", "Cancel") == 1 then
				self:quit()
			end
		end)
		return false
	end,
	setup = function(self)
		ui.Application.setup(self)
		self:addInputHandler(ui.MSG_SIGNAL, self, self.handleAbort)
	end,
	cleanup = function(self)
		self:remInputHandler(ui.MSG_SIGNAL, self, self.handleAbort)
		ui.Application.cleanup(self)
	end,
	Children =
	{
		ui.Window:new
		{
			HideOnEscape = true,
			Children =
			{
				ui.Text:new 
				{ 
					Text = "GUI in parent task",
					Style = "font: ui-huge",
					Width = "free", Height = "free",
				}
			}
		}
	}
}:run()

-- exec.sleep(1000)
c:terminate()
