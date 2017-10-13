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

gr()
