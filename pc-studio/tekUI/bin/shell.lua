#!/usr/bin/env lua

local String = require "tek.lib.string"
local ui = require "tek.ui"
local load = loadstring or load

local function getline(line, color)
	line = String.new():set(line)
	line:setmetadata(color, 1, line:len())
	return line
end

local historyline = 2
local history = { 
-- 	'for i = 1, 10 do\nprint("Lua rocks")\nend',
}
local statement = { }

ui.Application:new
{
	AuthorStyles = [[
tek.ui.class.textedit.textlist {
	font: ui-fixed:18;
	margin: 0;
}
_textlist.textlist {
	font: ui-fixed:18;
	margin: 0;
}
tek.ui.class.canvas.textlist {
	border-style: groove;
	border-width: 2;
}
_editinput {
	font: ui-fixed:18;
}
]],
	Children =
	{
		ui.Window:new
		{
			HideOnEscape = true,
			Title = "Lua Shell",
			Orientation = "vertical",
			Width = 800,
			Children =
			{
				ui.TextList:new 
				{
					Id = "textlist",
					BGPens = { "#000", "#800", "#fff" },
					FGPens = { "#fff", "#fff", "#000" },
					HardScroll = true,
					Latch = "bottom",
					Height = "free",
					Style = "font: ui-fixed:18"
				},
				ui.Handle:new { },
				ui.Input:new
				{
					Id = "the-input",
					CursorStyle = "block",
					InitialFocus = true,
					MultiLine = true,
					Height = "free",
					Text = [[
-- Find the bug!
for i = 1, 10 do
	print "Lua rocks "..i.." times!"
end
print "Done."
]],
					
					run = function(self)
						local text = self:getText()
						if text ~= "" then
							local list = self:getById("textlist") 
							table.insert(history, text)
							historyline = #history + 1
							statement = { }
							for s in text:gmatch("([^\n]+)\n?") do
								table.insert(statement, s)
							end
							local all = table.concat(statement, "\n")
							local func, msg = load(all)
							if func then
								print = function(...)
									local t = { }
									for i = 1, select('#', ...) do
										table.insert(t, tostring(select(i, ...)))
									end
									list:addLine(table.concat(t, "\t"))
								end
								
								list:suspendUpdate()
								
-- 								for i = 1, #statement do
-- 									list:addLine(getline(statement[i], 3))
-- 								end
								
								local res, msg = pcall(function() func() end)
								list:releaseUpdate()
								
								statement = { }
								if not res and msg then
									self:getById("textlist"):addLine(getline(msg, 2))
								end
							elseif msg:match("'<eof>'$") then
								-- continue...
							else
								statement = { }
								local newline = getline(msg, 2)
								self:getById("textlist"):addLine(newline)
							end
							
						end
--						self:setValue("Text", "")
--						self:activate("click")
					end,
					
					handleKeyboard = function(self, msg)
						local code = msg[3]
						local qual = msg[6]
						local utf8code = msg[7]
						if code == 61458 and qual == 4 then
							-- cursor up
							historyline = math.max(historyline - 1, 1)
							local h = history[historyline]
							self:setValue("Text", h or "")
							self:cursorEOL()
						elseif code == 61459 and qual == 4 then
							-- down
							historyline = math.min(historyline + 1, #history + 1)
							local h = history[historyline]
							self:setValue("Text", h or "")
							self:cursorEOL()
						elseif code == 13 and qual == 4 then
							self:run()
						end
						return msg
					end,
				},
				ui.Button:new { 
					Text = "_Run (CTRL+Return)",
					onClick = function(self)
						self:getById("the-input"):run()
					end
				},
			}
		}
	}
}:run()
