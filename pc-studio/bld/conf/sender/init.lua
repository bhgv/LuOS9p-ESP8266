local ui = require "tek.ui"
local exec = require "tek.lib.exec"

--local Visual = ui.loadLibrary("visual", 4)
--local Visual = require "tek.lib.visual" --ui.loadLibrary("visual", 4)

--print(exec.getname())

--local on_timer = function()
--  status_time = 1
--end


return {
 task = nil,

  start = function(self, param)
    self.task = exec.run 
    {
      taskname = "sender",
      abort = false,
      func = function()
        local exec = require "tek.lib.exec"
        
        local MK = require "conf.controllers"
        --local MK = nil
        
        Log = require "conf.log"
        
        local lib = require "conf.sender.lib"
        
        local ascii85 = require "ascii85"
        
        local GFilters = {}
        
        local state = "stop"
        local int_state = "s"
        
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
        
        while cmd ~= "SENDER_STOP" do
          if MK then
            --print("int_state =", int_state) --, oks)
            
            if int_state == "s" then
              if stat_on then
                MK:status_query()
                int_state = "rs"
              else
                int_state = "m"
              end
            elseif int_state == "r" or int_state == "rs" then
              local msg
            --  repeat
                msg = lib:cnc_read_parse(MK, state)
            --  until(msg.msg or int_state == "r")
              
              --[=[
              if --[[msg and]] --[[msg.ok and]] MK.out_access and req_inc_cmd then
                if icmd < #gthread then
                  icmd = icmd + 1 
                else
                  state = "stop"
                  icmd = 1
                  exec.sendport("*p", "ui", "<MESSAGE>Stop")
                end
                req_inc_cmd = false
              end
              ]=]
              
              if msg.err then
                exec.sendport("*p", "ui", "<MESSAGE>" 
                              .. msg.msg:match("([^\u{a}\u{d}]+)") 
                              .. " (ln: " .. (send_from + icmd - 1) .. ")"
                              )
                state = "stop"
                --stat_on = true
                --icmd = icmd + 1
              end
              
              if msg.msg then --or int_state ~= "rs" then
              --  if msg.stat then --or int_state == "rs" then
                  int_state = "m"
              --  else --if msg.ok or msg.err or not msg.msg then
              --    int_state = "s"
              --  end
              elseif (not msg.msg) then
                local st, stms = Visual.getTime() 
                if int_state ~= "rs" and stat_on and (st - status_time) >= 1 then
                  status_time = st
                  int_state = "s"
                else
                  int_state = "m"
                end
              end
            
            elseif int_state == "m" then
              --print(#gthread, state)
              --print(MK.out_access)
              
              if --[[msg and]] --[[msg.ok and]] MK.out_access and req_inc_cmd and state == "run" then
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
                --print(is_resp_handled, oks, oks_max)
                if MK.out_access then --? is_resp_handled and oks < oks_max then
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
                  
                  --lib:display_tx( '(' .. (send_from + icmd - 1) .. ') ' .. cmd )
                  
                  if #GFilters > 0 then
                    local i,flt
                    for i,flt in ipairs(GFilters) do
                      cmd = flt:filter(cmd, flt.pars) or cmd
                      --print ("send", cmd)
                    end
                  end
                  
                  lib:display_tx( num_str .. cmd )

                  --if cmd == nil then
                  --  cmd = ""
                  --end
                  
                  MK:send(cmd .. '\n')
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
              --local mk = exec.waitmsg(2000)
              local prt = exec.waitmsg(2000)
              local bod = exec.waitmsg(2000)
              if prt ~= "" and bod ~= "" then
                --MK = MKs:get(mk)
                local r = MK:open(prt, bod)
                if r then
--                  if MK:open(prt, 0 + bod) then
--                    local out = MK:init()
--                    lib:split(out.msg, "[^\u{a}\u{d}]+", lib.display_rx_msg)
                    exec.sendport("*p", "ui", "<MESSAGE>Connected to " .. prt .. ", " .. bod)
                  else
                    MK = nil
                    exec.sendport("*p", "ui", "<MESSAGE>Can't connect to " .. prt)
--                  end
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
              if state == "stop" then
                sthread[#sthread + 1] = msg
                state = "single"
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
                    end
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
  end,

  newcmd = function(self, cmd)
    while cmd and not exec.sendmsg("sender", cmd) do
--      print("resend:", cmd)
    end
--    print("sent:", cmd)
  end

}
