#!/usr/bin/env lua
local exec = require "tek.lib.exec"
local guitask = exec.run(function()
	-- child task:
	local exec = require "tek.lib.exec"
	local ui = require "tek.ui"
	local app = ui.Application:new 
	{
		Children = 
		{
			ui.Window:new 
			{
				Title = "Child process",
				Width = "auto",
				Height = "auto",
				HideOnEscape = true,
				Children = 
				{
					ui.Tunnel:new { Height = "fill" },
					ui.Group:new 
					{
						Orientation = "vertical",
						Children = 
						{
							ui.Text:new 
							{
								Style = "text-align:left; padding: 10",
								Text = [[
This is a cross-platform example demonstrating a GUI running
in a child task.

If you click on the button below, the command will be sent
to the application's parent task. 

Clicking the close button will send the command 'quit'.]]
							},
							ui.Group:new 
							{
								Columns = 2,
								Children = 
								{
									ui.Text:new 
									{
										Class = "caption", Width="auto",
										Text = "Command:"
									},
									ui.Group:new 
									{
										Children = 
										{
											ui.Input:new 
											{
												Id = "input-message",
												MaxLength = 40,
												Text = "do_some_work"
											},
											ui.Button:new 
											{
												Width = "fill",
												Text = "Send Message",
												InitialFocus = true,
												onClick = function(self)
													local msg = self:getById("input-message"):getText()
													exec.sendmsg("*p", msg)
													if msg == "quit" then
														self.Application:quit()
													end
												end,
											},
										}
									},
									ui.Text:new
									{
										Class = "caption", Width="auto",
										Text = "Parent:"
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
											local userdata = msg[-1]
											self:setValue("Text", userdata)
											return msg
										end
									}
								}
							}
						}
					}
				}
			}
		}
	}
	exec.sendmsg("*p", "ready")
	app:run()
	exec.sendmsg("*p", "quit")
end)

-- parent task:

exec.waitmsg() -- "ready"
local wait_idle
guitask:sendport("ui", "idle")
while true do
	local msg = exec.waitmsg(2000)
	if msg then
		if msg == "quit" then
			break
		elseif msg == "do_some_work" then
			print "working..."
			guitask:sendport("ui", "doing some work.")
			exec.waittime(1000)
			guitask:sendport("ui", "doing some work..")
			exec.waittime(1000)
			guitask:sendport("ui", "doing some work...")
			exec.waittime(1000)
			guitask:sendport("ui", "doing some work....")
			exec.waittime(1000)
			guitask:sendport("ui", "work done.")
			print "work done."
			wait_idle = true
		else
			guitask:sendport("ui", "unknown command '" .. msg .. "'")
			wait_idle = true
		end
	else
		wait_idle = false
		guitask:sendport("ui", "idle")
	end
end

guitask:join()
