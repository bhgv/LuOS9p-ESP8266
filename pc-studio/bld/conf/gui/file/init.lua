local ui = require "tek.ui"

_G.Editor = ui.Input:new
              {
                AutoIndent = true,
--                ReadOnly = false,
                MultiLine = true,
                VisibleMargin = { 3, 2, 3, 2 },
                Height = "100%",
--                Class = "editor",
--                Style = "width:100%; height:100%",
            }
            

Editor.EditInput.getPathFileName = function(self)
	local path, file = self.FileName:match("^(.*)/([^/]+)$")
	if not path then
		local recentfile = self.InputShared.RecentFilename
		if recentfile then
			path, file = recentfile:match("^(.*)/([^/]+)$")
		end
		if not path then
			path = lfs.currentdir()
		end
	end
	return path, file	
end


            

local file_bar_pc = require "conf.gui.common.panel_file_pc"
local file_bar_mk = require "conf.gui.common.panel_file_mk"


local Group = ui.Group

--local display = dofile("conf/gui/display.lua")

return	Group:new -- file
{
  Orientation = "vertical",
  Children =
  {
    Group:new
    {
      Orientation = "horisontal",
      Children =
      {
--				   require "conf.gui.file.fopen",
      },
    },

    Group:new
    {
      Children =
      {
        require "conf.gui.file.popen",
        
        ui.Handle:new {  },
        
        ui.Group:new 
        {
          Children =
          {
            Group:new
            {
              Orientation = "vertical",
              Children =
              {
                Group:new
                {
                  Children =
                  {
                    file_bar_pc,
                    
                    --ui.Handle:new {  },
                    
                    file_bar_mk,
                  },
                },
                
                Group:new
                {
                  Orientation = "vertical",
                  Legend = "Editor",
                  Children =
                  {
                    require "conf.gui.common.editor_tools",
                    
                    Editor,
                  },
                },
              },
            },
          },
        },
      },
    },
  }
}
