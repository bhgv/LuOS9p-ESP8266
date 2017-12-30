local ui = require "tek.ui"
local exec = require "tek.lib.exec"

local ascii85 = require "ascii85"


--print(ui.ProgDir)

local complexcontrol = require "conf.gui.plugins.lib.complexcontrol"


local function preparePluginParamsDlg(plug_group, task, name, par_str)
  local k, v
  local chlds = {}
  local gr1, gr2
  
  --print(par_str)
  
  local pars = {}
  for k,v in par_str:gmatch("%s*([^=]+)=([^\n]*)[;\n]+") do
    pars[k] = v
  end
  
  for k,v in pairs(pars) do -- par_str:gmatch("%s*([^=]+)=%s*([^\n]+)[;\n]+") do
    if k:match("^<") then
      --print(k,v)
      local wgt = complexcontrol(k, v)
      
      table.insert(chlds, wgt)
      --table.insert(chlds, ui.Text:new{Text=k,})
      --table.insert(chlds, ui.Input:new{Text=v,})
    end
  end
  gr1 = ui.Group:new{Orientation="vertical", Children=chlds,}

  chlds = {}
  for k,v in pairs(pars) do -- par_str:gmatch("%s*([^=]+)=%s*([^\n]*)[;\n]+") do
    if not k:match("^<") then
      table.insert(chlds, ui.Text:new{Text=k,})
      table.insert(chlds, ui.Input:new{Text=v,})
    end
  end
  gr2 = ui.Group:new{Columns=2, Children=chlds,}
  
  dlg = {}
  if name and name ~= "" then
    dlg[1] = ui.Text:new{Text=name,Class = "caption",}
  end
  if gr1 then
    table.insert(dlg, gr1)
  end
  if gr2 then
    table.insert(dlg, gr2)
  end
  --local b_name = "Execute"
  --if
  table.insert(dlg, 
                ui.Button:new{
                    plug_task = task, 
                    par_complex = gr1,
                    par_group = gr2, --chlds, --gr,
                    Text = (plug_group == "Filter" and "Attach") or "Execute",
                    onClick = function(self)
                      ui.Button.onClick(self)
                      
                      local out = ""
                      local lst
                      if self.par_group then
                        lst = self.par_group:getChildren()
                        --print("exec", #lst)
                        if lst and #lst > 0 then
                          local i
                          --out = ""
                          for i = 1, #lst, 2 do
                            out = out .. lst[i].Text .. "=" .. 
                                              ascii85.encode(lst[i+1]:getText()) 
                                              .. "\n"
                          end
                        end
                      end
                      
                      if self.par_complex then
                        lst = self.par_complex:getChildren()
                        --print("exec", #lst)
                        if lst and #lst > 0 then
                          local i
                          --out = out or ""
                          for i = 1, #lst do
                            local it = lst[i]
                            if it.int_type then
                              --print(it.int_type .. "=" .. it.control_param)
                              local v = ascii85.encode(it.control_param)
                              out = out .. it.int_type .. "=" .. v .. "\n"
                            end
                          end
                        end
                      end
                      
                      if out then
                        --print(out)
                        exec.sendmsg(self.plug_task, "<EXECUTE>" .. out)
                      end
                    end,
                }
  )
  
  if plug_group == "Filter" then
    table.insert(dlg, 
                ui.Button:new{
                    plug_task = task, 
                    par_complex = gr1,
                    par_group = gr2, --chlds, --gr,
                    Text = "Detach",
                    onClick = function(self)
                      ui.Button.onClick(self)
                      
                      --if out then
                        --print(out)
                        exec.sendmsg(self.plug_task, "<DETACH>")
                      --end
                    end,
                }
    )
  end
  
  table.insert(dlg, 
                ui.Button:new{
                    plug_task = task, 
                    par_complex = gr1,
                    par_group = gr2, --chlds, --gr,
                    Text = "Save as default",
                    onClick = function(self)
                      ui.Button.onClick(self)
                      
                      local out
                      local lst
                      if self.par_group then
                        lst = self.par_group:getChildren()
                        --print("exec", #lst)
                        if lst and #lst > 0 then
                          local i
                          out = ""
                          for i = 1, #lst, 2 do
                            out = out .. lst[i].Text .. "=" .. lst[i+1]:getText() .. "\n"
                          end
                        end
                      end
                      
                      if self.par_complex then
                        lst = self.par_complex:getChildren()
                        --print("exec", #lst)
                        if lst and #lst > 0 then
                          local i
                          out = out or ""
                          for i = 1, #lst do
                            local it = lst[i]
                            if it.int_type then
                              --print(it.int_type .. "=" .. it.control_param)
                              out = out .. it.int_type .. "=" .. it.control_param .. "\n"
                            end
                          end
                        end
                      end
                      
                      if out then
                        --print(out)
                        exec.sendmsg(self.plug_task, "<SAVE>" .. out)
                      end
                    end,
                }
  )
  return ui.Group:new{Orientation = "vertical",Legend="Parameters:",Children = dlg, }
end



local NumberedList = require "conf.gui.classes.numberedlist"

local function cr_plugpars(grp)
  local chldrn = {}
  --print (grp)
  if grp == "Filter" then
    local lst = ui.ListView:new {
              --Height = 200,
              VSliderMode = "auto",
              HSliderMode = "auto",
              Headers = { "n", "Filter" },
              Child = ui.Lister:new
              {
                SelectMode = "single",
                
                EnabledFilterPos = {},
                
        				ListObject = NumberedList:new{},
                
                onSelectLine = function(self)
                  ui.Lister.onSelectLine(self)
                  local line = self.SelectedLine --self:getItem(self.SelectedLine)
                  --print("on sel ln", line)
                  if line and self.EnabledFilterPos[line] then
                    local trd = self.EnabledFilterPos[line].trd
                    --print("trd", trd)
                    if trd then
                      --print("click")
                      exec.sendmsg(trd, "<CLICK>")
                    end
                  end
                end,
                    
                setup = function(self, app, win)
                  ui.Lister.setup(self, app, win)
                  --print(app, win)
                  app:addInputHandler(ui.MSG_USER, self, self.msgUser)
                end,
                
                cleanup = function(self)
                  ui.Lister.cleanup(self)
                  self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
                end,
                
                msgUser = function(self, msg)
                  local ud = msg[-1]
                  --print(ud)
                  local typ, trd, nm = ud:match("<PLUGIN>" 
                                        .. "([^<]*)"
                                        .. "<NAME>"
                                        .. "([^<]*)"
                                        .. "<ATTACHED>" 
                                        .. "(.*)"
                  )
                  if typ then
                    --print(ud)
                    --print( typ, trd, nm )
                    local i, it
                    for i = #self.EnabledFilterPos,1,-1 do
                      it = self.EnabledFilterPos[i]
                      if it and it.trd == trd then
                        table.remove(self.EnabledFilterPos, i)
                        self:remItem( i )
                      end
                    end
                    self:addItem( {{"", nm}} )
                    i = self:getN()
                    table.insert(self.EnabledFilterPos, i, {trd=trd, })
                  else
                    typ, trd, nm = ud:match("<PLUGIN>" 
                                          .. "([^<]*)"
                                          .. "<NAME>"
                                          .. "([^<]*)"
                                          .. "<DETACHED>" 
                                          .. "(.*)"
                    )
                    if typ then
                      --print("detach", typ, trd, nm )
                      local i, it
                      for i = #self.EnabledFilterPos,1,-1 do
                        it = self.EnabledFilterPos[i]
                        if it and it.trd == trd then
                          table.remove(self.EnabledFilterPos, i)
                          self:remItem( i )
                        end
                      end
                    end
                  end
                  
                  return msg
                end,

              },
    }
    table.insert(chldrn, lst)
  end
  local ppars =
          ui.Group:new
          {
            Orientation = "vertical",
          --  Width = 75+120+32,
            Pars_group = grp,
            Children = chldrn,
            setup = function(self, app, win)
                      ui.Group.setup(self, app, win)
                      app:addInputHandler(ui.MSG_USER, self, self.msgUser)
            end,
            cleanup = function(self)
                      ui.Group.cleanup(self)
                      self.Application:remInputHandler(ui.MSG_USER, self, self.msgUser)
            end,
            msgUser = function(self, msg)
                      local ud = msg[-1]
                      --print("ud", ud)
                      local grp, task, nxt_lvl
                      grp, task, nxt_lvl = ud:match("^<PLUGIN>([^<]*)<NAME>([^<]*)(<.*)")
                      --print(self.Pars_group, grp, task, nxt_lvl)
                      if grp and self.Pars_group == grp and nxt_lvl then
                        if nxt_lvl:match("^<REM PARAMS>") then
                          --self:setValue("Children", {})
                          
                          --[[
                          local lst = self:getChildren()
                          local i,v 
                          for i = #lst,1,-1 do
                            v = lst[i]
                            self:remMember(v)
                          end
                          ]]
                          if self.Param_grp then
                            self:remMember(self.Param_grp)
                            self.Param_grp = nil
                          end
                        elseif nxt_lvl:match("^<SHOW PARAMS>") then
                          local name
                          name, nxt_lvl = nxt_lvl:match("^<SHOW PARAMS>([^<]*)<BODY>(.*)")
                          
                          local param_grp = preparePluginParamsDlg(self.Pars_group, task, name, nxt_lvl)
                          self.Param_grp = param_grp
                          self:addMember( param_grp )
                        --[[
                        elseif nxt_lvl:match("^<GCODE>") then
                          nxt_lvl = nxt_lvl:match("^<GCODE>(.*)")
                          local t = {}
                          local ln
                          --print(nxt_lvl)
                          for ln in nxt_lvl:gmatch("([^\n]+)\n") do
                            table.insert(t, ln)
                          end
                          GTXT = t
                          initialiseEditor()
                          do_vparse()
                        ]]
                        end
                      end
                      
                      return msg
            end,
          }
  
  Plugins.Gui.PlugPars[grp] = ppars
  
  return ppars
end


return cr_plugpars --Plugins.Gui.PlugPars
