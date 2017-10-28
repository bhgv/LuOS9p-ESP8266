
p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio




net.setup("TP-LINK", "")

net.httpd()

gui.setFont(6)
gui.run "menu/main.lua"

oled.cls()
oled.setFont(3)

y=0


-- [[
ip = net.lookup "mult.ru"

s = sock.socket(0)
er = sock.connect(s, ip, 80)
if er == 0 then
    er = sock.send(s, "GET http://mult.ru/ \r\nUser-Agent: Linux\r\n\r\n")
    if er>=0 then
	repeat
	    txt, r = sock.recv(s, "*l")
	    print(txt)
	    oled.print(0,y,txt)
	    y = y + 8
	until(txt=="" and y>=64)
    end
end

sock.close(s)

oled.draw()
oled.setContrast(0)

oled.setFont(0)
-- ]]

