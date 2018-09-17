
dofile("wifi_ssid_pass.lua")

p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio


print( "gui set font" )
gui.setFont(6)

is_sta = true
sel_mnu = 0

--[[
adc1_itr=function() 
    while true do 
	print(adc[1]); 
	thread.sleep(1); 
    end 
end
-- ]]


--[[
gg_param = {0,0,0,0}

	local f = io.open("/etc/gg.txt")
	if f then
	    local i, s
	    for i = 1,4 do
		s = f:read("n")
--		print(i, s)
		gg_param[i] = 0 + s
	    end
	    f:close()
	    f = nil
	    i = nil
	    s = nil
	end


--gui.run "menu/gg.lua"

gg_param = nil

thread.sleep(1)
]]

while sel_mnu ~= 6 do
    gui.run "menu/select.lua"
    thread.sleep(1)

    if sel_mnu == 4 then
	gui.run "menu/soldr.lua"

    elseif sel_mnu == 2 then
	net.sta(WIFI_SSID, WIFI_PASS)

	is_sta = true

	gui.run "menu/sta.lua"

	while net.localip() == 0 do end
    elseif sel_mnu == 1 then
	thread.start( httpd.loop )
	thread.sleep(1)
	gui.run "menu/httpd.lua"

	httpd.stop()
	thread.sleep(1)

    elseif sel_mnu == 3 then
	gui.run "menu/main.lua"

    elseif sel_mnu == 5 then
	gg_param = {0,0,0,0}
	local f = io.open("/etc/gg.txt")
	if f then
	    local i, s
	    for i = 1,4 do
		s = f:read("n")
--		print(i, s)
		gg_param[i] = 0 + s
	    end
	    f:close()
	    f = nil
	    i = nil
	    s = nil
	end
	gui.run "menu/gg.lua"
	gg_param = nil

	thread.sleep(1)

    elseif sel_mnu == 7 then
	thread.start( styx.loop )
	thread.sleep(1)
	gui.run "menu/styx.lua"

	styx.stop()
	thread.sleep(1)

    end

end

oled.cls()
oled.setFont(3)

y=0
--[[
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

thread.sleep(2)
oled.cls()



