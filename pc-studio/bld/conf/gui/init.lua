
local ui = require "tek.ui"
local exec = require "tek.lib.exec"


ports = {}
bauds = {}

f = io.lines("conf/ports.txt")
repeat
    p = f(); if p then table.insert(ports, {{p}} ) end
until(p == nil)

f = io.lines("conf/bauds.txt")
repeat
    p = f(); if p then table.insert(bauds, {{p}} ) end
until(p == nil)


local Group = ui.Group
local Slider = ui.Slider
local Text = ui.Text

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")


require "conf.gui.common"


--MKs = require "conf.controllers"
--MK = nil
--MKStep = 10
--MKstate = nil


App = ui.Application:new {
--      AuthorStyleSheets = "gradient", --"klinik", --"desktop" --
      AuthorStyleSheets = "klinik", --"desktop" --
}


local page_captions = {
          "_File", 
--          "_Control", 
}
local pages = {
          require("conf.gui.file"),
--          require("conf.gui.control"),
}

--[[
local k,v 
for k,v in pairs(_G.Flags.Plugins.Groups) do
  --print(k, v)
  if type(v) == "table" and #v > 0 then
    table.insert(page_captions, k)
    local pg = require("conf.gui.plugins")
    table.insert(pages, pg(k))
  end
end
]]

--table.insert(page_captions, "_Editor")
--table.insert(pages, require("conf.gui.edit"))
--table.insert(page_captions, "_Terminal")
--table.insert(pages, require("conf.gui.terminal"))
--table.insert(page_captions, "_Showroom")
--table.insert(pages, require("conf.gui.showroom"))
  


local window = ui.Window:new
{
    Orientation = "vertical",
    Width = 1024,
    Height = 650,
    MinWidth = 800,
    MinHeight = 600,
    MaxWidth = "none", 
    MaxHeight = "none",
    Title = "luOS9p studio",
    Status = "hide",
    HideOnEscape = true,
    SizeButton = true,
    Children =
    {
      ui.PageGroup:new
      {
        PageCaptions = page_captions,
        Style = "font:Vera/b:18;",
        Children = pages,
      },
  
      ui.Gauge:new
      {
        Min = 0,
        Max = 1,
        Value = 0,
        Width = "free",
        Height = 5, --"auto",
      
        show = function(self)
			ui.Gauge.show(self)
			self.Application:addInputHandler(ui.MSG_USER, self, self.msgUser)
	end,
        hide = function(self)
			ui.Gauge.hide(self)
			self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
	end,
        msgUser = function(self, msg)
                      local ud = msg[-1]
                      --print(ud)
                      local max = ud:match("<CMD GAUGE SETUP>(%d+)")
                      if max ~= nil then
                        --print("max=" .. max)
                        self:setValue("Max", 0+max)
                        self:setValue("Value", 0)
                      else
                        local pos = ud:match("<CMD GAUGE POS>(%d+)")
                        if pos ~= nil then
                          --print("pos=" .. pos)
                          self:setValue("Value", 0+pos)
                        end
                      end
                      --self:setValue("Text", userdata)
                      return msg
                    end
      },

      ui.Group:new
      {
        Children =
        {
          ui.Text:new
          {
            Style = "font:/b:10; color:gray;", --olive;", --navy;",
            Text = "Not Connected",
            setup = function(self, app, win)
				ui.Text.setup(self, app, win)
				app:addInputHandler(ui.MSG_USER, self, self.msgUser)
		end,
            cleanup = function(self)
				ui.Text.cleanup(self)
				self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
		end,
            msgUser = function(self, msg)
                      local ud = msg[-1]
                      --print("ud", ud)
                      local cmd = ud:match("<MESSAGE>(.*)")
                      if cmd then
                        --print("cmd=" .. cmd)
                        if 
                          cmd:match("^error:") 
                        then
                          MKstate = "PAUSE"
                        elseif 
                          cmd:match("^Pause")
                        then
                          MKstate = "PAUSE"
                          cmd = "status: " .. cmd
                        elseif 
                          cmd:match("^Stop")
                        then
                          MKstate = "STOP"
                          cmd = "status: " .. cmd
                        elseif 
                          cmd:match("^Run")
                        then
                          MKstate = "RUN"
                          cmd = "status: " .. cmd
                        end
                        
                        self:setValue("Text", cmd)
                      end
                      return msg
                    end,

          },
        
          ui.Text:new
          {
            Id = "status main",
            Style = "font:/b:10; color:gray;", --olive;", --navy;",
          },
          
        }
      }
    },
  
    setup = function(self, app, win)
                      ui.Window.setup(self, app, win)
                      app:addInputHandler(ui.MSG_USER, self, self.msgUser)
    end,
    cleanup = function(self)
                      ui.Window.cleanup(self)
                      self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
    end,
    msgUser = function(self, msg)
                      local ud = msg[-1]
                      --print("ud", ud)
                      local g
                      if ud:match("^<GCODE>") then
                        g = ud:match("^<GCODE>(.*)")
                        local t = {}
                        local ln
                        --print(nxt_lvl)
                        for ln in g:gmatch("([^\n]+)\n") do
                          table.insert(t, ln)
                        end
                        GTXT = t
                        initialiseEditor()
                        do_vparse()
                      end
                      
                      return msg
    end,
}


ui.Application.connect(window)
App:addMember(window)
window:setValue("Status", "show")
App:run()

return 0
