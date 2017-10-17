--i2c.setup(2,14)

p=print

for k in pairs(os) do _G[k]=os[k] end


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

--gr()

--pwm.init()
--gpio.init()

--pwm.open_drn(true)
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
