--local stf=dofile "gstuff/gg_ctl.lua"

local is_changed = false

local acts_uni={
    lft=function(k) is_changed = true; gg_param[k]=gg_param[k]-1; if gg_param[k] < 0 then gg_param[k] = 0 end end, 
    rgt=function(k) is_changed = true; gg_param[k]=gg_param[k]+1; end, 
    lpg=function(k) is_changed = true; gg_param[k]=gg_param[k]-10; if gg_param[k] < 0 then gg_param[k] = 0 end end, 
    rpg=function(k) is_changed = true; gg_param[k]=gg_param[k]+10; end, 
--    ok=function(k) gui.gotop(); end, 
}


local acts_save={
-- [[
    ok=function(k) 
	local f = io.open("/etc/gg.txt", "w")
	local i, p
	for i = 1,4 do
	    p = _G.gg_param[i]
	    if p then
		f:write('' .. p .. '\n')
	    end
	end
	f:flush()
	f:close()

	is_changed = false
    end, 
--]]
}


local ind_t=function(y, k) 
    local p, ph, pm
    p = gg_param[k]

    pm = p % 60
    ph = (p - pm)/60

    oled.print(80, y, string.format("%2d:%2d", ph, pm) ); 
end


local ind_changed=function(y, k) 
    local s

    if is_changed then
	s = '*'
    else
	s = ' '
    end

    oled.print(100, y, s ); 
end



local menu_gg_cnf={
	{name="Lgt t:", act=acts_uni, par=1, ind_t=ind_t },
	{name="Drk t:", act=acts_uni, par=2, ind_t=ind_t },
	{name="Wet t:", act=acts_uni, par=3, ind_t=ind_t },
	{name="Dry t:", act=acts_uni, par=4, ind_t=ind_t },
	{name="Save config", act=acts_save, par=nil, ind_t=ind_changed },
	{name=" ..main menu"},
}
--stf=nil

return menu_gg_cnf
