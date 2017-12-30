-------------------------
-- copyright by bhgv 2016
-------------------------


return {
  name = "runtime custom gcode changer",
  type = "plugin", 
  subtype = "Filter",
  gui = "button",
  image = nil,
  symbol = "\u{e0b9}",
  
  exec = function(self, cmd, pars)
    --for k,v in pairs(pars) do
    --  print("execk ", k, v)
    --end
    gcode = cmd
    local foo = load(pars['<LUA EDITOR>gcode'])
    
    return foo() --or cmd
  end,
}
