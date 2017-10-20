--i2c.setup(2,14)

p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio

--[[
d=u8g.disp()

f = function(d,i)
    d:drawCircle(i,30,15); 
end


gr = function()
local i
while true do 
    for i=15,104,4 do 
	d:draw( f, i )
    end 
end
end
]]


--gr()

--pwm.init()
--gpio.init()

--pwm.open_drn(true)
--[[
p("is OD", pwm.is_open_drn())

while 1 do
    for i=0,1000 do
	pwm.DC1=i/10.0
	pwm.DC2=i/10.0
	pwm.DC3=i/10.0
	pwm.SGN0=i/10.0
	pwm.SGN1=100.0-(i/10.0)
	--print(i)
    end
end
]]

gui=dofile("gui/init.lua")
--gui=require "gui"

dac=0
acts_pwm={lft=function(k) pwm[k]=pwm[k]-1 end, rgt=function(k) pwm[k]=pwm[k]+1 end, lpg=function(k) pwm[k]=pwm[k]-10 end, rpg=function(k) pwm[k]=pwm[k]+10 end, }
ind_pwm=function(d, y, k) d.print(100, y, string.format("%d%%", math.ceil(pwm[k]) ) ); end
ind_menu=function(d, y, k) d.triangle(118, y, 118, y+7, 125, y+4); end
--acts_dac={lft=function() adc.DAC=pwm[k]-1 end, rgt=function(k) pwm[k]=pwm[k]+1 end, lpg=function(k) pwm[k]=pwm[k]-10 end, rpg=function(k) pwm[k]=pwm[k]+10 end, }

gui.new_menu("menu/main")

-- [[
gui.menu_loop()
--]]
