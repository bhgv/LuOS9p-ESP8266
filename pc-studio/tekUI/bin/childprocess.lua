#!/usr/bin/env lua

local posix = require "posix"
local char = string.char
local byte = string.byte

local fdr1, fdw1 = posix.pipe() -- pipe from child to parent
local fdr2, fdw2 = posix.pipe() -- pipe from parent to child

local cid = posix.fork()
if cid == -1 then
	print "fork failed"
	return
end

if cid ~= 0 then
	-- in parent process:
	
	-- close unneeded ends of pipes:
	posix.close(fdw1) 
	posix.close(fdr2)

	-- send msg to gui: these messages are strings separated by newline
	-- and dispatched to the GUI as messages of the type MSG_USER.
	
	local function sendmsg_gui(msg)
		posix.write(fdw2, msg .. "\n")
	end
	
	local wait_idle
	sendmsg_gui("idle")
	while true do
		if posix.rpoll(fdr1, wait_idle and 2000 or -1) == 1 then
			local len = byte(posix.read(fdr1, 1))
			local msg = posix.read(fdr1, len)
			if msg == "quit" then
				break
			elseif msg == "do_some_work" then
				sendmsg_gui("doing some work.")
				posix.sleep(1)
				sendmsg_gui("doing some work..")
				posix.sleep(1)
				sendmsg_gui("doing some work...")
				posix.sleep(1)
				sendmsg_gui("doing some work....")
				posix.sleep(1)
				sendmsg_gui("work done.")
				wait_idle = true
			else
				sendmsg_gui("unknown command '" .. msg .. "'")
				wait_idle = true
			end
		else
			wait_idle = false
			sendmsg_gui("idle")
		end
	end
	posix.wait(cid)
	return
end

-- in child process:
-- close unneeded ends of pipes:
posix.close(fdr1)
posix.close(fdw2)

-- send msg to parent: we use a slightly different protocol here,
-- first the length in bytes, then the message body.

local function sendmsg_parent(msg)
	posix.write(fdw1, char(msg:len()) .. msg)
end

local ui = require "tek.ui"

-- make the read-end of our pipe known:
ui.MsgFileNo = fdr2 

ui.Application:new
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
This is a POSIX-specific example demonstrating a GUI running
in a child process.

If you click on the button below, the command will be sent
to the application's parent process. 

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
												sendmsg_parent(msg)
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
}:run()
	
sendmsg_parent("quit")
