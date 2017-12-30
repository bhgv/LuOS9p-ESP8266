
local ui = require "tek.ui"



return function(tmpl, v)
  local wgt
  
  local pars_str = tmpl:match('<LUA EDITOR>([^<>=]*)') or ""
  print(pars_str)
  
  local lua_editor = ui.Input:new {
					--Id = "the-input",
					CursorStyle = "block",
					InitialFocus = true,
					MultiLine = true,
					Height = "free",
					Text = v or "",
  }
  
  wgt = ui.Group:new {
      Legend = name or "Enter a lua code:",
      Orientation = "vertical",
      
      int_type = tmpl, --"<FILE>",
      control_param = v or "", 
      
      Children = {
				lua_editor,
        
        ui.Button:new {
          Text = "_Compile",
          onClick = function(self)
            ui.Button.onClick(self)
            
            local code = lua_editor:getText()
            
            local pars = {
              [pars_str] = '',
            }
            
            local foo, err = load(
                                  code 
                                  , "custom filter"
                                  , 't'
                                  , pars
                    )
            if not foo then
              print(err)
              wgt.control_param = ''
            else
              local comp = string.dump(foo)
              wgt.control_param = comp
            end
          end,
        },
      },
  }

  return wgt
end

