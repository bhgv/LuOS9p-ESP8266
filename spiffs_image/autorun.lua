
p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio


--gui=dofile "gui/init.lua"
--gui=require "gui"


gui.run "menu/main.lua"

oled.cls()
y=0

net.setup("TP-LINK", "")

s = net.socket(0)
ip = net.lookup "mult.ru"
er = net.connect(s, ip, 80)
if er == 0 then
    er = net.send(s, "GET http://mult.ru/ \r\nUser-Agent: Linux\r\n\r\n")
    if er>=0 then
	repeat
	    txt, r = net.recv(s, "*l")
	    oled.print(0,y,txt)
	    y = y + 8
	until(txt=="" and y>=64)
    end
end

net.close(s)

oled.draw()
oled.setContrast(0)

