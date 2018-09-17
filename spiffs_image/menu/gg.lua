--local stf=dofile "gstuff/gg_ctl.lua"
local stf_mnu=dofile "gstuff/menu.lua"


--local param = {0,0,0,0}
local curr = {0,0,0,0}
local is_lvl = false

local lvl_i, lvl_n = 0, 0

local is_light = true
local is_wet = true

local dly = 0


local ind_t=function(y, k) 
    local p, c, ch, cm, kl, prel, kw, prew
    local nm, pic

    if is_wet then
	pic = "wet_"
	kw = 3
	prew = "Wet"
    else
	pic = "dry_"
	kw = 4
	prew = "Dry"
    end

    if is_light then
	pic = pic .. "light"
	kl = 1
	prel = "Day"
    else
	pic = pic .. "dark"
	kl = 2
	prel = "Ngt"
    end

    local w,h = oled.PBM(10, y, "/ico/" .. pic .. ".pbm");
    pic = nil

    local y2 = y + (h >> 1)
    local x2 = 10 + w + 4

    oled.setFont(0)

    p = gg_param[kl]
    c = curr[kl]

    c = p - c
    cm = c % 60
    ch = (c - cm)/60

    oled.print(x2, y2 - 20, string.format(prel .. ":%2d:%2d", ch, cm) )
    prel = nil

    p = gg_param[kw]
    c = curr[kw]

    c = p - c
    cm = c % 60
    ch = (c - cm)/60

    oled.print(x2, y2 - 5, string.format(prew .. ":%2d:%2d", ch, cm) )

    if is_lvl then
	prew = "full"
    else
	prew = "empty"
    end

    oled.print(x2, y2 + 11, "Wtr:" .. prew )

    return h+2
end


local cb_foo = function()
    local p, c, k

    dly = dly + 1
    if dly < 30 then
	if lvl_i < 3 then
	    lvl_n = lvl_n + adc[1]
	    lvl_i = lvl_i + 1
	else
	    is_lvl = (lvl_n < 40.0)
--print(lvl_n, is_lvl, dly)
	    lvl_n = 0
	    lvl_i = 0
	end

	if is_lvl then
	    pwm.DC2 = 0
	else
	    is_wet = false
--		pwm.DC1 = 0
	    pwm.DC2 = 30
	end

	return false
    end
    dly = 0

--	    is_lvl = not gpio.bExtKey
--[[
    if lvl_i < 2 then
	lvl_n = lvl_n + adc[1]
	lvl_i = lvl_i + 1
    else
	is_lvl = (lvl_n < 3.0)
	lvl_n = 0
	lvl_i = 0
    end

    if is_lvl then
	pwm.DC2 = 0
    else
	is_wet = false
--	pwm.DC1 = 0
	pwm.DC2 = 30
    end
]]

    if is_light then
	k = 1
    else
	k = 2
    end

    p = gg_param[k]
    c = curr[k]
    if c < p then
	c = c + 1
    else
	c = 0
	is_light = not is_light
    end
    curr[k] = c

    if adc[2] > 30 then
	gpio.AC3 = is_light
	if is_light then
	    pwm.LED1 = 2
--	    pwm.LED2 = 2
	else
	    pwm.LED1 = 0
--	    pwm.LED2 = 0
	end
    end

    if is_lvl then
	if is_wet then
	    k = 3
	else
	    k = 4
	end
	p = gg_param[k]
	c = curr[k]

	if c < p then
	    c = c + 1
	else
	    c = 0
	    is_wet = not is_wet
	end
	curr[k] = c

	if is_wet then
	    pwm.DC1 = c*20
	else
	    pwm.DC1 = 0
	end
    else
	pwm.DC1 = 0
    end

    return true;
end


local menu_gg={
	{name="", menu="menu/gg_conf.lua", ind_t=ind_t },
--	{name="Water level", act=nil, par=nil, ind_t=ind_lvl },
--	{name="Config", menu="menu/gg_conf.lua", ind_t=stf_mnu.ind, },
	{name="Exit", act={ok=function() gui.exit(); end, }, },
	{name=" ..redraw"},
	cb=cb_foo,
}
--stf=nil
stf_mnu = nil


return menu_gg
