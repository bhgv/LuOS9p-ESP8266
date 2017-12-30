
ui = require "tek.ui"


return ui.Group:new
{
  Orientation = "horisontal",
  Legend = "MK file ops",
  Children = 
  {
--[[
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
]]
    symBut(
      "\u{E03c}",
      function(self)
        local app = self.Application
        app:addCoroutine(function()
                local lister = require "conf.gui.common.dirlist_mk"
--                local NumberedList = require "conf.gui.classes.numberedlist"
                local status, path, select = app:requestFile
                {
                  Lister = lister:new{
                      DisplayMode = "all",
                      BasePath = "",
                      Path = self.old_path or "/",
                      Kind = "requester",
                      SelectMode = "single",
                      --Location = args.Location,
                      --SelectText = args.SelectText,
                      --FocusElement = args.FocusElement,
                      --Filter = args.Filter,
                  },
                  
                  Path = self.old_path or "/", 
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
                  if path == '/' then
                    GFNAME = path .. select[1]
                  end
                  
                  app:getById("status main"):setValue("Text", "Opening " .. GFNAME)
                  
                  local txt, size, t = "", 0, ""
                  
                  MK:send("HIDEUARTOTPUT")
                  MK:send("SINGLE")
                  MK:send("io.send('" .. GFNAME .. "');\r\n")
                  MK:recv()

                  repeat
                    MK:send("SINGLE")
                    MK:send("C\n")

                    t = MK:recv()
                    size = string.byte( t )
                    if size > 0 then
                      t = t:sub(2)
                      txt = txt .. t
                    end
                  until(t == nil or size == 0)
                  MK:send("SHOWUARTOTPUT")
                    
                  if Editor then
                    --Editor:setValue("Text", txt)
                    Editor.EditInput:clearBookmarks()
                    Editor.EditInput:newText(txt)
                    Editor.EditInput.FileName = GFNAME
                  end
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
            local pth, fle = "", ""
            if Editor.EditInput.FileName then
              pth, fle = Editor.EditInput:getPathFileName()
            end
            
            local lister = require "conf.gui.common.dirlist_mk"
            local status, path, select = app:requestFile
            {
                Lister = lister:new{
                      DisplayMode = "all",
                      BasePath = "",
                      Path = self.old_path or "/",
                      Kind = "requester",
                      SelectMode = "single",
                      --Location = args.Location,
                      --SelectText = args.SelectText,
                      --FocusElement = args.FocusElement,
                      --Filter = args.Filter,
                },
                
                Location = fle,
                Title = "Save File As..",
                SelectText = "Save",
                FocusElement = "location",
                
                Path = self.old_path or "/", 
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
              if path == '/' then
                GFNAME = path .. select[1]
              end
              
              app:getById("status main"):setValue("Text", "Saving " .. GFNAME)
              
              local txt, size, t, i, l = "", 0, "", 1, 0
              
              txt = Editor.EditInput:getText()
              
              MK:send("HIDEUARTOTPUT")
              MK:send("SINGLE")
              MK:send("io.receive('" .. GFNAME .. "');\r\n")
              
              t = MK:recv()
              
              MK:send("SINGLE")
              MK:send("S")

              repeat
                repeat
                  t = MK:recv()
                until(t and t == "C\n")
                
                l = #txt - i + 1
                if l > 255 then
                  l = 255
                end
                
                MK:send("SINGLE")
                MK:send( string.char( l ) )
                MK:send("SINGLE")
                MK:send( txt:sub(i, i + l - 1) )
                
                i = i + l
              until(i >= #txt)
              
              repeat
                t = MK:recv()
              until(t and t == "C\n")
              
              MK:send("SINGLE")
              MK:send( string.char( 0 ) )
              
              MK:send("SHOWUARTOTPUT")
              
              app:getById("status main"):setValue("Text", GFNAME)
            end
          end 
        )
      end
    ),
  }
}

