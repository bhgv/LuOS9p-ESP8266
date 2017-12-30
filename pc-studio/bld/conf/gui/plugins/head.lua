
local ui = require "tek.ui"
local exec = require "tek.lib.exec"

local lastGroup

local plugBut = function(conf, taskname)
  local crfoo
  local ico
  local issmall = false
  local isnosym = false
  
  if conf.symbol then
    crfoo = symBut
    ico = conf.symbol
  elseif conf.nosymbol then
    crfoo = symBut
    ico = conf.nosymbol
    isnosym = true
  elseif conf.symbolsm then
    crfoo = symButSm
    ico = conf.symbolsm
    issmall = true
  end
  
  local but = crfoo(
      ico, 
      function(self)
        local conf = self.plug_conf
        local task = self.plug_task
        
        exec.sendmsg(task, "<CLICK>")
      end,
      "olive",
      isnosym
  )
  but.plug_conf = conf
  but.plug_task = taskname
  
  if 
      (not lastGroup) or 
      (lastGroup.Rows and not issmall) or
      (issmall and not lastGroup.Rows)
  then
    local tmpl = {
      Children = {but,}
    }
    if issmall then tmpl.Rows = 2 end
    lastGroup = ui.Group:new(tmpl)
    return lastGroup
  else
    table.insert(lastGroup.Children, but)
  end

  return nil
end

--[[
local chlds = {}
local i, plg, conf
for i,plg in ipairs(_G.Flags.Plugins) do
  conf = plg.conf
  if conf.gui == "button" then
    local b = plugBut(conf, plg.taskname)
    if b then
      table.insert(chlds, b)
    end
  end
end
]]

--[[
--Plugins.Gui.Header = ui.Group:new
local hdr = ui.Group:new
{
  Children = chlds,
}
]]
--return hdr --Plugins.Gui.Header

return function(grp)
  local chlds = {}
  local i, j, plg, conf
  local gr_tab = _G.Flags.Plugins.Groups[grp]
  for j = 1,#gr_tab do
    i = gr_tab[j]
    plg = _G.Flags.Plugins[i]
  --for i,plg in ipairs(_G.Flags.Plugins) do
    conf = plg.conf
    if conf.gui == "button" then
      local b = plugBut(conf, plg.taskname)
      if b then
        table.insert(chlds, b)
      end
    end
  end

  local hdr = ui.Group:new
  {
    Children = chlds,
  }

  lastGroup = nil
  
  Plugins.Gui.Headers[grp] = hdr
  
  return hdr --Plugins.Gui.Header
end
