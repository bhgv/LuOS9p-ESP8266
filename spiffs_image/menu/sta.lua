
local sta = net.scan();

local menu_sta = {};
local i, j;

j = 1;
for i = 1,#sta do
    local ssid = sta[i].ssid;
    if ssid ~= "" then
	j = #menu_sta + 1;
        menu_sta[j] = {
	    name = "run '" .. ssid .. "'", 
	    act = {
		ok=function() 
		    WIFI_SSID = ssid;
		    is_sta = true;
		    net.sta(ssid, "");
		    gui.exit(); 
		end,
	    },
	};
    end
end
j = #menu_sta + 1;

menu_sta[ j ] = {name="Ap '" .. AP_SSID .. "'", act={ ok=function() is_sta=false; net.ap(AP_SSID, AP_PASS); gui.exit(); end, }, };
j = j + 1;
menu_sta[ j ] = {name="exit Sta/Ap switch", act={ ok=function() gui.exit(); end, }, };
j = j + 1;
menu_sta[ j ] = {name=" ..redraw"};

sta = nil; j = nil; i = nil;

return menu_sta;
