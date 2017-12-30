
local exec = require "tek.lib.exec"



return {
  split = function(self, str, mask, cb_foo)
      local s 
      for s in string.gmatch(str, mask) do
        cb_foo(self, s)
      end
  end,
        
  display_rx = function(self, s)
      if s and s ~= "" then
        exec.sendport("*p", "ui", "<PORT RX>" .. s) 
      end
  end,
        
  display_rx_msg = function(self, s) 
      if s and s ~= "" then
        local t = "<PORT RX MSG>" .. s
        exec.sendport("*p", "ui", t) 
      end
  end,
        
  display_tx = function(self, s) 
      if s then
        exec.sendport("*p", "ui", "<PORT TX>" .. s) 
      end
  end,

  cnc_stat = function(self, stat)
      if stat and
        stat.w.x and stat.w.y and stat.w.z and
        stat.m.x and stat.m.y and stat.m.z
      then
        exec.sendport("*p", "ui", "<STATUS><wX>" .. stat.w.x
                                      .. "<wY>" .. stat.w.y
                                      .. "<wZ>" .. stat.w.z
                                      .. "<mX>" .. stat.m.x
                                      .. "<mY>" .. stat.m.y
                                      .. "<mZ>" .. stat.m.z
            )
      end
  end,
--[[
  cnc_status_read = function(self, MK)
              local out
              repeat
                out = MK:read()
              until(out.msg)
              --print ("msg=", out.msg)
              if out.stat then
                self:cnc_stat(MK:status_parse(out.msg))
              elseif out.ok or out.err then
                print("!!!!!!!!!!!!!!!!!!!: ", out.ok, out.err)
                --int_state = "m"
              end
              return out
  end,
]]
  cnc_read_parse = function(self, MK, state)
              local out = MK:read()
              if out.msg then
                if out.stat then
                  self:cnc_stat(MK:status_parse(out.msg))
                else --if out.ok or out.err then
                  if state == "run" and (out.ok or out.err) then
                    self:split(out.msg, "[^\u{a}\u{d}]+", self.display_rx)
                  else
                    self:split(out.msg, "[^\u{a}\u{d}]+", self.display_rx_msg)
                  end
                end
              end
              return out
  end,

}