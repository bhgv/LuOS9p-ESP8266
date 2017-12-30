
ui = require "tek.ui"


return ui.Group:new
{
  Orientation = "horisontal",
  Legend = "PC file ops",
  Children = 
  {
    symBut(
      "\u{E052}",
      function(self)
        if Editor then
          Editor.EditInput:newText("")
          Editor.EditInput:setValue("FileName", nil)
          --Editor.EditInput:newText(txt)
        end
      end
    ),

    symBut(
      "\u{E03c}",
      function(self)
        local app = self.Application
        app:addCoroutine(function()
                --List = require "tek.class.list"
--                local NumberedList = require "conf.gui.classes.numberedlist"
                local status, path, select = app:requestFile
                {
                  Path = self.old_path or os.getenv("HOME"), 
                  SelectMode = 
                --		    "multi",
                        "single",
                  DisplayMode = 
                        "all" 
                --		    or "onlydirs"
                }
                if status == "selected" then
                  self.old_path = path
                  
                  GFNAME = path .. "/" .. select[1]
                  app:getById("status main"):setValue("Text", "Opening " .. GFNAME)
                  --print(status, path, table.concat(select, ", "))
                  --local f = io.open(GFNAME, "r")
                  --if f ~= nil then
                  --  local txt = f:read("*a")
                  --  GTXT = txt
                    
                    if Editor then
                      --Editor:setValue("Text", txt)
                      Editor.EditInput:clearBookmarks()
                      Editor.EditInput:loadText( GFNAME )
                      --Editor.EditInput:newText(txt)
                    end
                   -- f:close()
                  --end
                  app:getById("status main"):setValue("Text", GFNAME)
                end
            end 
        )
      end
    ),

    symBut(
      "\u{E03d}",
      function(self)
        local app = self.Application
        app:addCoroutine(function()
            local pth, fle
            if Editor.EditInput.FileName then
              pth, fle = Editor.EditInput:getPathFileName()
            end
            pth = pth or self.old_path or os.getenv("HOME")
            
            local status, path, select = app:requestFile
              {
                Path = pth, 
		
    Location = fle,
		Title = "Save File As..",
		SelectText = "Save",
		FocusElement = "location",
    
		--Width = 580,
    
                SelectMode = 
                --		    "multi",
                        "single",
                DisplayMode = 
                        "all" 
                --		    or "onlydirs"
              }
            
            if status == "selected" then
                  self.old_path = path
                  
                  GFNAME = path .. "/" .. select[1]
                  app:getById("status main"):setValue("Text", "Saving " .. GFNAME)
                  --print(status, path, table.concat(select, ", "))
                  --local f = io.open(GFNAME, "r")
                  --if f ~= nil then
                  --  local txt = f:read("*a")
                  --  GTXT = txt
                    
                    if Editor then
                      --Editor:setValue("Text", txt)
                      --Editor.EditInput:clearBookmarks()
                      Editor.EditInput:saveText( GFNAME )
                      --Editor.EditInput:newText(txt)
                    end
                   -- f:close()
                  --end
                  app:getById("status main"):setValue("Text", GFNAME)
                end
            end 
        )
      end
    ),
  }
}

