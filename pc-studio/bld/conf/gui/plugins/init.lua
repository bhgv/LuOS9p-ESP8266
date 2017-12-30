local ui = require "tek.ui"
local Group = ui.Group

--local display = dofile("conf/gui/display.lua")


return function(grp)
  local hd = require "conf.gui.plugins.head"
  local ppars = require "conf.gui.plugins.plugpars"
  local pg = 
    Group:new -- file
		{
		    Orientation = "vertical",
		    Children =
		    {
          Group:new
          {
              Orientation = "horisontal",
              Children =
              {
                hd(grp),
              },
          },
          Group:new
          {
              Children =
              {
                ppars(grp),
                ui.Handle:new {  },
                ui.Group:new 
                {
                    Children =
                    {
                      DisplayBlock,
                    }
                },
              },
          }
		    }
		}
    
  return pg
end
