
ui = require "tek.ui"


symBut = function(txt, foo, color, isnosym)
  local font = ":18" --"Vera:18"
  --print (isnosym,txt)
  if not isnosym then
    font = "ui-icons:20"
  end
  
  local style = "font:" .. font .. "; width:24; color:olive;" --olive;" --navy;"
  local par = {}
--  if par.Style then
--    style = style .. par.Style
--  end
  par.Style = style
  par.Text = txt
  par.onClick = function(self)
    ui.Button.onClick(self)
    --		self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)
    foo(self)
  end
  
  return ui.Button:new(par)
end


symButSm = function(txt, foo, color)
  local style = "font:ui-icons:12; width:12; color:olive;" --olive;" --navy;"
  local par = {}
--  if par.Style then
--    style = style .. par.Style
--  end
  par.Style = style
  par.Text = txt
  par.Class = "caption"
  par.onClick = function(self)
    ui.Button.onClick(self)
    --		self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)
    foo(self)
  end
  
  return ui.Button:new(par)
end


--require "conf.gui.common.display_block"


local posInd = function(typ)
  local out = ui.Text:new {
      Width=60,
      setup = function(self, app, win)
			ui.Text.setup(self, app, win)
                      --print(app, win)
			app:addInputHandler(ui.MSG_USER, self, self.msgUser)
	end,
      cleanup = function(self)
			ui.Text.cleanup(self)
			self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
	end,
      msgUser = function(self, msg)
                      local ud = msg[-1]
                      --print("ud", ud)
                      if ud:match("<STATUS>") then
                        local cmd = ud:match("<" .. typ .. ">([^<]*)")
                        if cmd ~= nil then
--                          cmd = cmd:match("([^\n]*)")
                          --print("cmd=" .. cmd)
                          self:setValue("Text", cmd)
                        end
                      end
                      return msg
                    end,
  }
  
  return out
end





StatPort = ui.Group:new
{
  Orientation = "vertical",
  Legend = "Status",
  Children = 
  {
    ui.Text:new
    {
      Class = "caption",
      Width = 150,
      Style = "font:/b:14; color:navy;", --olive;", --navy;",
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
                          --MKstate = "PAUSE"
                        elseif 
                          cmd:match("^Pause")
                        then
                          --MKstate = "PAUSE"
                          cmd = "status: " .. cmd
                        elseif 
                          cmd:match("^Stop")
                        then
                          --MKstate = "STOP"
                          cmd = "status: " .. cmd
                        elseif 
                          cmd:match("^Run")
                        then
                          cmd = "status: " .. cmd
                        end
                        
                        self:setValue("Text", cmd)
                      end
                      return msg
                    end,

    },
    
--[[
    ui.Group:new
    {
      Rows = 3,
      Columns = 4,
      Children = 
      {
          ui.Text:new{Class = "caption",Width=60, Text="WPos:"},
          posInd("wX"), posInd("wY"), posInd("wZ"), 
          ui.Text:new{Class = "caption",Width=60, Text="MPos:"},
          posInd("mX"), posInd("mY"), posInd("mZ"), 
          ui.Text:new{Class = "caption",Width=60, },
          ui.Button:new{Width=60, Text="x = 0",
            onClick = function(self)
              ui.Button.onClick(self)
              MK:set_xyz{x=0,}
            end,
          }, 
          ui.Button:new{Width=60, Text="y = 0",
            onClick = function(self)
              ui.Button.onClick(self)
              MK:set_xyz{y=0,}
            end,
          }, 
          ui.Button:new{Width=60, Text="z = 0",
            onClick = function(self)
              ui.Button.onClick(self)
              MK:set_xyz{z=0,}
            end,
          }, 
      },
    },
]]

  }
}


SetStatus = function(status)
  StatPort.Children[1]:setValue("Text", status)
end

