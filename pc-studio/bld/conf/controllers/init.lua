
local ui = require "tek.ui"
local exec = require "tek.lib.exec"


local rs232 = require('periphery').Serial
local PORT = nil

local out_access

local read_timeout = 200

local port_task = nil


local ctrlrs = {
--    Grbl = require "conf.controllers.grbl",
}


local send = function()
      if not out_access then return false end
      
      PORT:write(cmd)
      
      return true
end

local recv = function()
      if not out_access then return nil end
      
      buf = PORT:read(256, read_timeout) --200)
      
      return buf
end



local port_loop = function(param)
    port_task = exec.run 
    {
      taskname = "sender",
      abort = false,
      func = function()
        local exec = require "tek.lib.exec"
        
        local rs232 = require('periphery').Serial
        local MK = require "conf.controllers"
        local PORT = nil
        
        Log = require "conf.log"
        
        local lib = require "conf.sender.lib"
        
        local ascii85 = require "ascii85"
        
        local GFilters = {}
        
        local state = "stop"
        local int_state = "m" --"s"
        
        local req_inc_cmd = false
        
        local stat_on = true
--        local cnc_stat_ctr = 0
        
        --print(exec.getname())
        
        local icmd = 1
        local gthread = {}
        local cmd = ""
        local msg = nil
        
        local sthread = {}
        
        local send_from = 1
        
        --local Visual = ui.loadLibrary("visual", 4)
        local Visual = require "tek.lib.visual"
        
--        self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.update)
        local status_time, stms = Visual.getTime()
    
                  
        local is_out_to_term = true
    
        local out_access = false
    
        local _send = function( cmd )
          if cmd == nil or PORT == nil then return false end
          
          PORT:write(cmd)
          
          return true
        end

        local _recv = function()
          local buf = ""
          local t
          
          if not PORT then return nil end
          
          t = PORT:read(256, 50) --read_timeout) --200)
          
          while t ~= nil and t ~= "" do
            buf = buf .. t
            t = PORT:read(256, 50) --read_timeout) --200)
          end
          
          return buf
        end

        
        while true or cmd ~= "SENDER_STOP" do
          if PORT ~= nil then
            
            if int_state == "r" then
              local msg
            --  repeat
              msg = _recv()
            --  until(msg.msg or int_state == "r")
              
              if msg ~= nil and msg ~= "" then
                if is_out_to_term then
                  local pt
                  for pt in string.gmatch(msg, "([^\n\r]+)" ) do
                      pt = string.gsub(pt, "\t", "    ")
                      lib:display_rx_msg(pt)
                  end
                else
                  while not exec.sendmsg("*p", msg) do
                  end
                end
              elseif msg == "" and not is_out_to_term then
                  while msg and not exec.sendmsg("*p", "") do
                  end
              end

              int_state = "m"
            
            else --if int_state == "m" then
              --print(#gthread, state)
              --print(MK.out_access)
              
              if out_access and req_inc_cmd and state == "run" then
                if icmd < #gthread then
                  icmd = icmd + 1 
                else
                  state = "stop"
                  icmd = 1
                  exec.sendport("*p", "ui", "<MESSAGE>Stop")
                end
                req_inc_cmd = false
              end
  
              if (
                  (#gthread >= icmd and state == "run") or 
                  (#sthread > 0 and state == "single")
              ) then
                if out_access then 
                  local num_str
                  if state == "single" then
                    --cmd = sthread[#sthread]
                    cmd = table.remove(sthread, #sthread)
                    if #sthread == 0 then
                      state = "stop"
                    end
                    num_str = ""
                  else
                    --icmd = icmd + 1
                    cmd = gthread[ icmd ]
                    --cmd = table.remove(gthread, 1)
                    
                    exec.sendport("*p", "ui", "<CMD GAUGE POS>" .. icmd
                    )
                    num_str = '(' .. (send_from + icmd - 1) .. ') '
                  end
                  
                  if #GFilters > 0 then
                    local i,flt
                    for i,flt in ipairs(GFilters) do
                      cmd = flt:filter(cmd, flt.pars) or cmd
                      --print ("send", cmd)
                    end
                  end
                  
                  --lib:display_tx( num_str .. cmd )

                  --if cmd == nil then
                  --  cmd = ""
                  --end
                  
                  _send( cmd )
                  --Log:msg(icmd .. ", " .. tostring(is_resp_handled) .. ", m cmd: " .. cmd)
                  
                  req_inc_cmd = true
                end
                
                int_state = "r"
              else
                int_state = "r" --"s"
              end
            end
          end
          
          msg = exec.waitmsg(20)
          
          if msg ~= nil then
            Log:msg("msg = " .. msg)
            --print("msg = " .. msg)
            if msg == "PORT" then
              local prt = exec.waitmsg(2000)
              local bod = exec.waitmsg(2000)
              if prt ~= "" and bod ~= "" then
                  PORT = rs232(prt, tonumber(bod) )
                  out_access = PORT ~= nil
                if PORT then
                    exec.sendport("*p", "ui", "<MESSAGE>Connected to " .. prt .. ", " .. bod)
                    state = "stop"
                    int_state = "r"
                  else
                    exec.sendport("*p", "ui", "<MESSAGE>Can't connect to " .. prt)
                end
              end
            elseif msg == "NEW" then
              gthread = {}
              icmd = 1
              state = "stop"
              req_inc_cmd = false
              exec.sendport("*p", "ui", "<MESSAGE>Stop")
            elseif msg == "CALCULATE" then
              exec.sendport("*p", "ui", "<CMD GAUGE SETUP>" .. #gthread)
              --state = "run"
            elseif msg == "PAUSE" then
              MK:pause()
              state = "stop"
              exec.sendport("*p", "ui", "<MESSAGE>Pause")
            elseif msg == "FILL" then
              stat_on = false
            elseif msg == "RESUME" then
              MK:resume()
              stat_on = true
              if state == "stop" then
                state = "run"
                exec.sendport("*p", "ui", "<MESSAGE>Run")
              end
            elseif msg == "STOP" then
              icmd = 1
              req_inc_cmd = false
              state = "stop"
              exec.sendport("*p", "ui", "<MESSAGE>Stop")
            elseif msg == "SINGLE" then
              msg = exec.waitmsg(200)
              if state == "stop" or  state == "single" then
                sthread[#sthread + 1] = msg
                --lib:display_rx(msg)
                
                state = "single"
                int_state = "m"
              end
            elseif msg == "SENDFROM" then
              msg = exec.waitmsg(200)
              if state == "stop" and msg then
                send_from = tonumber(msg)
              end
            elseif msg == "ADDFILTER" then
              local nm = exec.waitmsg(200)
              if nm then
                local pars_s = exec.waitmsg(200)
                local partab = {}
                local k,v
                for k,v in pars_s:gmatch("%s*([^=]+)=%s*([^\n]*)\n") do
                  partab[k] = ascii85.decode(v)
                end
                
                local i,flt
                for i = #GFilters,1,-1 do
                  flt = GFilters[i]
                  if flt.name == nm then
                    table.remove(GFilters, i)
                  end
                end
                table.insert(GFilters, {
                    name = nm,
                    pars = partab,
                    pars_s = pars_s,
                    
                    filter = function(self, cmd)
                      if cmd then
                        --exec.sendport("*p", "ui", "<FILTER>" self.name .. "<CMD>" .. cmd)
                        exec.sendmsg(self.name, "<FILTER>" .. cmd .. "<CMD>" .. self.pars_s)
                        return exec.waitmsg(500) or cmd
                      end
                      return ""
                    end,
                    
                })
                exec.sendmsg(nm, "<FILTER><ATTACHED>" .. (#GFilters))
              end
              
            elseif msg == "DELFILTER" then
              msg = exec.waitmsg(200)
              if msg then
                local i,flt
                local nm = msg
                for i = #GFilters,1,-1 do
                  flt = GFilters[i]
                  if flt.name == nm then
                    table.remove(GFilters, i)
                  end
                end
                exec.sendmsg(nm, "<FILTER><DETACHED>")
              end
              
            elseif msg == "HIDEUARTOTPUT" then
              is_out_to_term = false
              
            elseif msg == "SHOWUARTOTPUT" then
              is_out_to_term = true
              
            else
              while msg and msg ~= "FIN" do
                --print(msg)
                gthread[#gthread + 1] = msg
              --exec.sendport("main", "ui", "<CMD GAUGE SETUP 2>" .. #gthread)
                msg = exec.waitmsg(20)
              end
              --cmd = msg
            end
          end
        end
      end
    }
end


local newcmd = function(cmd)
--  print("newcmd ", cmd)
  while cmd and not exec.sendmsg("sender", cmd) do
--      print("resend:", cmd)
  end
end


port_loop()



return {
  open = function(port, speed)
          PORT = rs232(port, tonumber(speed) )
          out_access = PORT ~= nil
          
--          if PORT ~= nil then
--            port_loop()
--          end
          
          return PORT
  end,
  
  send = function(self, cmd)
--      if not out_access then return false end
      
      --PORT:write(cmd)
      newcmd(cmd)
      
      return true
  end,

-- [[
  recv = function(self)
--      if not out_access then return nil end
    local out = ""
    
    repeat
      local msg, sender, sig = exec.waitmsg(200)
      
      if msg then
        out = out .. msg
      end
    until( msg == nil or msg == "")
    
    return out
  end,
-- ]]
  
}
