#!/usr/bin/env lua

local socket
pcall(function() socket = require "socket" end)

local ui = require "tek.ui"
ui.Application:new
{
	up = function(self)
		if not socket then
			self:addCoroutine(function()
				self:easyRequest("socket missing", 
					"Sorry, this demo needs the Lua socket library.",
					"Quit")
				self:quit()
			end)
		end
	end,
			
	Children =
	{
		ui.Window:new
		{
			Title = "Socket message",
			Width = "auto",
			HideOnEscape = true,
			Orientation = "vertical",
			Children =
			{
				ui.Text:new
				{
					Style = "text-align:left",
					Text = [[
In this example, if you click on the button below, the
message 'hello' will be sent to the application's
localhost udp server on port 20000. Provided that the
server is running (see config, ENABLE_DGRAM), the
message should show up in the field below.]]
				},
				ui.Group:new
				{
					Columns = 2,
					Children =
					{
						ui.Text:new
						{
							Class = "caption", Width="auto",
							Text = "Message to send"
						},
						ui.Input:new
						{
							Id = "input-message",
							MaxLength = 40,
							Text = "hello"
						},
						ui.Text:new
						{
							Class = "caption", Width="auto",
							Text = "Destination address"
						},
						ui.Input:new
						{
							Id = "input-address",
							Style = "font: ui-fixed",
							Text = "127.0.0.1"
						},
						ui.Text:new
						{
							Class = "caption", Width="auto",
							Text = "Port number"
						},
						ui.Input:new
						{
							Id = "input-portnumber",
							Style = "font: ui-fixed",
							Text = "20000"
						},
					}
				},
				ui.Button:new
				{
					Width = "fill",
					Text = "Send Message",
					InitialFocus = true,
					onClick = function(self)
						local msg = self:getById("input-message"):getText()
						local dest = self:getById("input-address"):getText()
						local port = self:getById("input-portnumber"):getText()
						local client = socket.udp() 
						local res = client:sendto(msg, dest, port)
						client:close()
					end,
				},
				ui.Text:new
				{
					Width = "fill",
					show = function(self)
						ui.Text.show(self)
						self.Application:addInputHandler(ui.MSG_USER, self, self.msgUser)
					end,
					hide = function(self)
						ui.Text.hide(self)
						self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
					end,
					msgUser = function(self, msg)
						if msg[-1] == "quit" then
							self.Application:quit()
						end
						self:setValue("Text", msg[-1])
						return msg
					end
				}
			}
		}
	},
	
}:run()
