local ui = require "tek.ui"
local Group = ui.Group

return		Group:new -- term
		{
      Orientation = "vertical",
      Children =
      {
        Group:new
        {
          --Orientation = "horisontal",
          Children =
          {
    --        require "conf.gui.terminal.buttons",
            require "conf.gui.terminal.grun",
          },
        },
        Group:new
        {
          Children =
          {
            require "conf.gui.terminal.terminal",
            ui.Handle:new {  },
--[[
            ui.Group:new 
            {
              Children =
              {
                DisplayBlock,
              }
            },
]]
          },
        }
      }
}
