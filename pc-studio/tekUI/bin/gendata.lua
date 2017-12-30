#!/usr/bin/env lua

local exec = require "tek.lib.exec"

local PI = math.pi
local PI2 = PI*2
local SIN = math.sin

local s1 = 0
local s2 = 0
local x1 = 0
local x2 = 0

while true do

	local n = 256

	x1 = s1
	x2 = s2

	for cx = 0, n-1 do
		local y = ((x1 % 2) - 1 + SIN(x2) * 0.5) / 4 + 0.5
		y = y * 0x10000
		io.stdout:write(("%.0f "):format(y))
		x1 = x1 + 0.08
		x2 = x2 + 0.33
	end
	io.stdout:write("\n")

	s1 = s1 + 0.047
	s2 = s2 + 0.133
	if s2 > PI2 then
		s2 = s2 - PI2
	end

	-- wait given number of milliseconds:
	exec.sleep(10)

end
