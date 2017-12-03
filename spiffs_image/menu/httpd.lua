local menu_httpd = {
    {name="Ip=" .. net.localip("*s"), },
--    {name="", act={ok=function() end, }, },
    {name="exit HTTPd", act={ok=function() gui.exit(); end, }, },
    {name=" ..redraw"},
};

--local ok_ap, ok_sta;

--[[
local ap_sta_sw;

ap_sta_sw = function(is_ap)
    local t, foo;
    if not is_ap then
	t = "run Ap";
	foo = function() 
		net.ap("esp_tst", "");
		ap_sta_sw(true);
	end;
    else
	t = "conn '" .. WIFI_SSID .. "'";
	foo = function() 
		net.sta(WIFI_SSID, "");
		ap_sta_sw(false);
	end;
    end
    print("try", t)
    menu_httpd[2].name=t;
    menu_httpd[2].act.ok = foo;
end
]]

--[[
ok_ap = function(k) 
    net.ap("esp_tst", "");
    ap_sta_sw(false, 2);
end

ok_sta = function(k) 
    net.sta(WIFI_SSID, "");
    ap_sta_sw(true, 2);
end
]]

--[[
local ip0 = net.localip("*n");

ap_sta_sw( ip0 == 0 );
ip0 = nil;
]]

return menu_httpd
