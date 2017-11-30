
WIFI_SSID = "TP-LINK"
WIFI_PASS = ""

AP_SSID = "luos_test_ap"
AP_PASS = ""

p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio


oled.cls()
oled.print(5, 10, "try to connect to:")
oled.print(27, 22, '"' .. WIFI_SSID .. '"')

net.sta(WIFI_SSID, WIFI_PASS)

local i = 0
local ip1, ip2, ip3, ip4 = 0, 0, 0, 0

while i < 20 do
    oled.print (32, 40, "  " .. i .. " s  ")
    oled.draw()

    thread.sleep(1)
    ip1, ip2, ip3, ip4 = net.localip "*n"
    if ip1 ~= 0 or ip2 ~= 0 then
	break;
    else
	i = i + 1
    end
end

ip1, ip2 = net.localip "*n"
if ip1 == 0 and ip2 == 0 then
    net.ap(AP_SSID, AP_PASS)
end

oled.cls()
oled.draw()


gui.setFont(6)

menus_loop=true
while menus_loop do
htd = thread.start( httpd.loop )
--httpd.loop()

--net.httpd()
gui.run "menu/httpd.lua"
httpd.stop()

oled.cls()
oled.draw()

--thread.sleep(1)
--thread.stop(htd)
gui.setFont(6)
gui.run "menu/main.lua"
end


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

