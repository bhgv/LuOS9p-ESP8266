
dofile("wifi_ssid_pass.lua")

p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio


print( "gui set font" )
gui.setFont(6)

sta_loop = true

while sta_loop do
    sta_loop = false

    net.sta(WIFI_SSID, WIFI_PASS)

    is_sta = true

    print( "Sta/Ap select" )
    gui.run "menu/sta.lua"

    while net.localip() == 0 do end

--[[
if is_sta then
    print( "Try connect '" .. WIFI_SSID .. "'" )
    oled.cls()
    oled.print(5, 10, "try to connect to:")
    oled.print(27, 22, '"' .. WIFI_SSID .. '"')

    net.sta(WIFI_SSID, WIFI_PASS)

    local i = 0
    local ip1, ip2, ip3, ip4 = 0, 0, 0, 0

    is_sta = false
    while i < 40 do
	oled.print (32, 40, "  " .. i .. " s  ")
        oled.draw()

	thread.sleep(1)
        ip1, ip2, ip3, ip4 = net.localip "*n"
	if ip1 ~= 0 then
	    is_sta = true
	    break
        else
	    i = i + 1
	end
    end
    oled.cls()
--    oled.draw()

--    ip1 = net.localip "*n"
--    if ip1 == 0 then
    if not is_sta then
	print( "can't connect to '" .. WIFI_SSID .. "', start AP '" .. AP_SSID .. "'" )
	net.ap(AP_SSID, AP_PASS)
    end
end
]]

    thread.sleep(1)

    menus_loop=true
    while menus_loop do
	print( "run httpd + httpd_menu" )
	htd = thread.start( httpd.loop )
	thread.sleep(1)
	gui.run "menu/httpd.lua"

	print( "stop httpd & run main menu" )
	httpd.stop()

	gui.run "menu/main.lua"
    end
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

