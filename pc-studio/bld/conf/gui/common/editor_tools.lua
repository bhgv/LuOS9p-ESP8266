
ui = require "tek.ui"


local find_input = ui.Input:new { }

return ui.Group:new
{
  Orientation = "horisontal",
  --Legend = "Editor toolbar",
  Children = 
  {
    symButSm(
      "\u{E021}",
      function(self)
        if Editor then
          local cy = Editor.EditInput.CursorY
          Editor.EditInput:addBookmark(cy)
          --Editor.EditInput:newText(txt)
          Editor.EditInput:setActive(true)
        end
      end
    ),

    symButSm(
      "\u{e0cf}\u{e022}",
      function(self)
        if Editor then
          local cy = Editor.EditInput.CursorY
          Editor.EditInput:removeBookmark(cy)
          Editor.EditInput:setActive(true)
        end
        --app:getById("status main"):setValue("Text", GFNAME)
      end
    ),

    symButSm(
      "\u{e022}\u{e00d}",
      function(self)
        if Editor then
          local cy = Editor.EditInput.CursorY
          Editor.EditInput:prevBookmark(cy)
          Editor.EditInput:setActive(true)
        end
      end
    ),

    symButSm(
      "\u{e00e}\u{e022}",
      function(self)
        if Editor then
          local cy = Editor.EditInput.CursorY
          Editor.EditInput:nextBookmark(cy)
          Editor.EditInput:setActive(true)
        end
      end
    ),
    
    find_input,

    symButSm(
      "\u{e0e2}",
      function(self)
        if Editor then
          local ed = Editor.EditInput
          local cy = ed.CursorY
          --Editor.EditInput:nextBookmark(cy)
          local text = find_input:getText()
print(text)
          
          ed.FindText = text
          ed:clearMark()
          ed:markWord(text)
          ed:find(text)
          
          ed:setActive(true)
          --[[
          	Editor.EditInput:textRequest(self.FindText, function(text)
		self.FindText = text
		self:clearMark()
		self:markWord(text)
		self:find(text)
	end, "find", "find text", "do find")
]]
        end
      end
    ),
    
  }
}

