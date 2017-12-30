
local lfs = require"lfs"
local exec = require "tek.lib.exec"

local plugs_no = 0



local test_run_plugin = function(plug_path, sep_pars)
  print("test_run_plugin", plug_path)
  local task = exec.run(
    {
      taskname = "plug_" .. plugs_no,
      abort = false,
      func = function()
        local exec = require "tek.lib.exec"
        
        local ascii85 = require "ascii85"
        
        local tab2str = function(tab)
              local s = ""
              local k1, v1
              for k1,v1 in pairs(tab) do
                v1 = tostring(v1)
                v1 = v1:gsub("=", "\\u{3d}")
                v1 = v1:gsub("&", "\\u{26}")
                s = s .. k1 .. "=" .. v1 .. "\n"
              end
              return s
        end
        
--[[
        local new_G = {
        }
        
        local function get_upvalue(fn, search_name)
          local idx = 1
          while true do
            local name, val = debug.getupvalue(fn, idx)
            if not name then break end
            if name == search_name then
              return idx, val
            end
            idx = idx + 1
          end
        end
      
        local function set_upvalue(fn, name, val)
          debug.setupvalue(fn, get_upvalue(fn, name), val)
        end
]]

        local plug_path = arg[1]
        local sep_pars = (arg[2] == "true")
        
        --print (sep_pars)
        
        local f, fpar
        
        if sep_pars then
          fpar = loadfile(plug_path .. "/params.lua")
          
          if not fpar then 
            s = "can't load: " .. plug_path .. "/params.lua"
            print(s)
            return s 
          end
          
          f = loadfile(plug_path .. "/init.lua")
        else
          f = loadfile(plug_path)
        end
        
        if not f then 
          s = "can't load: " .. plug_path .. "/init.lua"
          print(s)
          return s 
        end
        
--        set_upvalue(f, "_ENV", new_G)

        local noerr_par, conf_par
        if sep_pars then
          noerr_par, conf_par = pcall(fpar)
          
          if not noerr_par then 
            print(conf)
            print(debug.traceback()) 
            return "can't execute: " .. plug_path .. "/params.lua"
          end
        end
        
        local noerr, conf = pcall(f)
        
        if not noerr then 
          print(conf)
          print(debug.traceback()) 
          return "can't execute: " .. plug_path .. "/init.lua"
        end
        
        --conf.__plugin_path = plug_path
        --conf.params = conf_par
        
        print("loaded plugin: " .. conf.name .. ", (" .. plug_path .. ")")
        
        if not conf.subtype then conf.subtype = "Stuff" end
        
        if conf.type == "plugin" then
          local k, v, s
          s = ""
          for k,v in pairs(conf) do 
            if k == "params" then
              if not sep_pars then
                conf_par = v
              end
              --[[
              s = s .. "params="
              local k1, v1
              for k1,v1 in pairs(v) do
                v1 = tostring(v1)
                v1 = v1:gsub("=", "\\u{3d}")
                v1 = v1:gsub("&", "\\u{26}")
                s = s .. k1 .. "\\u{3d}" .. v1 .. "\n"
              end
              s = s .. "&"
              ]]
            elseif k ~= "exec" then
              v = tostring(v)
              v = v:gsub("=", "\\u{3d}")
              v = v:gsub("&", "\\u{26}")
              s = s .. k .. "=" .. v .. "&" 
            end
          end
          --print("s =", s)
          --exec.sendport("*p", "ui", "<PLUGIN><CONNECT>" .. s)
          exec.sendmsg("*p", s)
        else
          exec.sendmsg("*p", "")
          return "wrong config. plugin: " .. plug_path
        end
        
        
        
        local msg
        while msg ~= "QUIT" do 
          msg = exec.waitmsg(2000)
          if msg then
            --print(msg)
            
            if msg == "<CLICK>" then
              exec.sendport("*p", "ui", 
                "<PLUGIN>" 
                .. conf.subtype 
                .. "<NAME>"
                .. exec.getname() 
                .. "<REM PARAMS>"
              )
              if conf_par then -- conf.params then
                local s = conf_par --conf.params
                if type(s) == "table" then
                  s = tab2str(s)
                end
                exec.sendport("*p", "ui", 
                  "<PLUGIN>" 
                  .. conf.subtype
                  .. "<NAME>"
                  .. exec.getname() 
                  .. "<SHOW PARAMS>" 
                  .. (conf.name or "")
                  .. "<BODY>"
                  .. s
                )
              end
            
            elseif conf.exec and msg:match("^<EXECUTE>") then
              --print(msg)
              local pars = msg:match("^<EXECUTE>(.*)")
              --print("conf.subtype =", conf.subtype)
              if conf.subtype == "Filter" then
                exec.sendmsg("sender", "ADDFILTER")
                exec.sendmsg("sender", exec.getname())
                exec.sendmsg("sender", pars)
                
                local m = exec.waitmsg(200)
                local i = m:match("<FILTER><ATTACHED>(%d*)")
                --print ("inst", i)
                
                exec.sendport("*p", "ui", 
                  "<PLUGIN>" 
                  .. conf.subtype
                  .. "<NAME>"
                  .. exec.getname() 
                  .. "<ATTACHED>" 
                  .. (conf.name or "")
                )
              else
                local partab = {}
                local k,v
                for k,v in pars:gmatch("%s*([^=]+)=%s*([^\n]*)\n") do
                  partab[k] = ascii85.decode(v)
                end
                local noerr, out = pcall(conf.exec, self, partab)
                if not noerr then 
                  print(out)
                  print(debug.traceback()) 
                end
              end
--            end

            elseif conf.exec and msg:match("^<DETACH>") then
              if conf.subtype == "Filter" then
                exec.sendmsg("sender", "DELFILTER")
                exec.sendmsg("sender", exec.getname())
                
                local m = exec.waitmsg(200)
                local i = m:match("<FILTER><DETACHED>") --(%d*)")
                --print ("inst", i)
                
                exec.sendport("*p", "ui", 
                  "<PLUGIN>" 
                  .. conf.subtype
                  .. "<NAME>"
                  .. exec.getname() 
                  .. "<DETACHED>" 
                  .. (conf.name or "")
                )
              end

            elseif conf.exec and msg:match("^<SAVE>") then
              local pars = msg:match("^<SAVE>(.*)")
              local partab = {}
              local k,v
              --print(plug_path .. "/params.lua")
              local fsv = io.open(plug_path .. "/params.lua", "w")
              if fsv then
                fsv:write("return {\n")
                for k,v in pars:gmatch("%s*([^=]+)=%s*([^\n]*)\n") do
                  fsv:write("\t['" .. k .. "'] = '" .. v .. "',\n")
                end
                fsv:write("}\n")
              end
              
            elseif conf.exec and msg:match("^<FILTER>") then
              --print(msg)
              local cmd, pars_s = msg:match("<FILTER>([^<]*)<CMD>(.*)")  
                local partab = {}
                local k,v
                for k,v in pars_s:gmatch("%s*([^=]+)=%s*([^\n]*)\n") do
                  partab[k] = ascii85.decode(v)
                end
              local noerr, out = pcall(conf.exec, self, cmd, partab)
              if not noerr then 
                print(out)
                print(debug.traceback()) 
              else
                if not out then out = cmd end
                exec.sendmsg("sender", out)
              end
            end
            
          end
        end
        
        return msg
      end,
    },
    plug_path,
    tostring(sep_pars)
  )
  
  local msg = exec.waitmsg(1000)
  --print("msg =", msg)
  if (not msg) or msg == "" then
    task:join()
    exec.getsignals("actm")
  else
    local k, v
    local t = {}
    for k,v in msg:gmatch("([^%s=]+)=([^&]+)&") do
      v = v:gsub("\\u{3d}", "=")
      v = v:gsub("\\u{26}", "&")
      t[k] = v
    end
    
    table.insert(_G.Flags.Plugins, {
        taskname = "plug_" .. plugs_no, 
        conf = t,
    })
  
    local grps = _G.Flags.Plugins.Groups
    if t.subtype then
      if not grps[t.subtype] then
        grps[t.subtype] = {}
      end
      table.insert(grps[t.subtype], #_G.Flags.Plugins)
    else
      table.insert(grps.Stuff, #_G.Flags.Plugins)
    end
    plugs_no = plugs_no + 1
  end
end


return {
  collect_plugins = function(self)
    local path = _G.Flags.Plugins_path
    plugs_no = 0
    
    for file in lfs.dir(path) do
      if file ~= "." and file ~= ".." and file ~= "init.lua" then
        local f = path..'/'..file
        --print ("\t "..f)
        local attr = lfs.attributes (f)
        assert (type(attr) == "table")
        if attr.mode == "directory" then
          if 
              lfs.attributes(f .. "/init.lua") 
              and lfs.attributes(f .. "/params.lua") 
          then
            test_run_plugin(f, true) 
          end
        elseif file:match("%.lua$") then
        --  for name, value in pairs(attr) do
        --    print (name, value)
        --  end
          test_run_plugin(f, false)
        end
      end
    end
  end,

}