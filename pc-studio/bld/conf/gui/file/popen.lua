local ui = require "tek.ui"
local List = require "tek.class.list"

--print(ui.ProgDir)

local mk_nm = ""
local port_nm = ""
local baud_nm = ""


wjt_portslist = ui.PopList:new
      {
        Id = "Port:",
        Width = 140,
        SelectedLine = 1,
        ListObject = List:new
        {
          Items = {}
        },
        onSelect = function(self)
          ui.PopList.onSelect(self)
          local item = self.ListObject:getItem(self.SelectedLine)
          if item then
            port_nm = item[1][1]
          end
        end,
      }


wjt_baudslist = ui.PopList:new
      {
        Id = "Baud:",
        Width = 140,
        SelectedLine = 1,
        ListObject = List:new
        {
          Items = {}
        },
        onSelect = function(self)
          ui.PopList.onSelect(self)
          local item = self.ListObject:getItem(self.SelectedLine)
          if item then
-- 											self:getById("japan-combo"):setValue("SelectedLine", self.SelectedLine)
            baud_nm = item[1][1]
--            self:getById("popup-show"):setValue("Text", item[1][1])
          end
        end,
      }

--[[
local get_mks = function()
-- [=[
  local lst = MKs:list()
  local i, mk
  local out = {}
  for i,mk in ipairs(lst) do
    out[#out + 1] = {{ mk }}
  end
  return out
-- ]=]
--    return {{{ "default" }}}
end
]]

local set_ports_bauds = function() --mk_type)
  local i, v
  local out = {}
  for i,v in ipairs(ports) do
    out[#out + 1] = v --{{ "" .. v }}
    print("port", v[1][1])
  end
  wjt_portslist:setList(List:new { Items = out })
  wjt_portslist:setValue("SelectedLine", 1)
  
  out = {}
  for i,v in ipairs(bauds) do
    out[#out + 1] = v --{{ "" .. v }}
    print("baud", v[1][1])
  end
  wjt_baudslist:setList(List:new { Items = out })
end

set_ports_bauds()




return ui.Group:new
{
  Orientation = "vertical",
--  Width = 75+120+32,
  Children = 
  {
    --StatPort,
    
    ui.Group:new
    {
      Legend = "UART Port",
      --Orientation = "vertical",
      
      Children = 
      {
        ui.Group:new
        {
          --Orientation = "vertical",
          --Columns = 2,
          --Rows = 3,
          Children = 
          {
            --[[
            ui.Text:new
            {
              Class = "caption",
              Width = 75,
              Text = "Port:",
            },
            ]]
            
            wjt_portslist,
            
--[[
            ui.Text:new
            {
              Class = "caption",
              Width = 75,
              Text = "Baud:",
            },
            wjt_baudslist,
]]
          }
        },

        symBut(
          "\u{E0df}",
          function(self)
-- [[
            MK:send("PORT")
            --Sender:newcmd(--[=[mk_nm]=] "grbl")
            MK:send(port_nm)
            MK:send(115200) --baud_nm)
            --SetStatus("Connected to: " .. port_nm)
            
            --MK = MKs:get(mk_nm)
            MKstate = "STOP"
--]]
--[[
            local r = MK.open(port_nm, baud_nm)
            print(r)
            if r then
--              StatPort.Children[1]:setValue("Text", "Connected to: " .. port_nm)
              SetStatus("Connected to: " .. port_nm)
            end
]]
          end
        ),
      },
    },
    
    require "conf.gui.terminal.terminal",

  }
}


