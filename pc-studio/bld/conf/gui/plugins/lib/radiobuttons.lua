
local ui = require "tek.ui"


return function(tmpl, init_val)
  local wgt
  local btns_cnf = {}
  local btns = {}
  local nm, v
  
  local grp_nm = tmpl:match('<RADIO>([^<]*)')
  
    wgt = ui.Group:new {
        Legend = grp_nm or "",
        Columns = 2,
        int_type = tmpl,
        control_param = init_val, 
  }

  
  for nm, v in tmpl:gmatch('<CASE>([^<]*)<VAL>([^<]*)') do
    table.insert( btns_cnf, {nm = nm, v = v,} )
    table.insert( btns, ui.Button:new {
        Text = nm,
        Class = "caption",
        Mode = "touch",
        radioGroup = wgt,
        Selected = (init_val == v),
        radioValue = v,
        
        onClick = function(self)
            ui.Button.onClick(self)
            
            local btns = self.radioGroup.Btns
            local val = self.radioValue
            
            self.radioGroup.control_param = val
            
            local i,b
            for i,b in ipairs(btns) do
              if b ~= self then
                b:setValue("Selected", false)
              end
            end
        end,
    })
  end
        --print(name, mask)
        
  wgt.Btns = btns
  wgt:setValue("Children", btns)
  
  return wgt
end

