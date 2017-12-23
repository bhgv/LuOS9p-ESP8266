
local solder_t = 0.0
local cur_t = adc[1]
local xtm = true

local pid_k = {p=20.0, i=-7.0, d =20.0, };

local pid_i = pid.new()
pid.limits(pid_i, 0, 100)
pid.tune(pid_i, pid_k.p, pid_k.i, pid_k.d)


local sol_f = function(i)
    solder_t=solder_t + i;
    if solder_t > 100.0 then 
	solder_t = 100.0;
    elseif solder_t < 0.0 then 
	solder_t = 0.0;
    end
    pid.dst(pid_i, solder_t);
end


local k_f = function(p, i)
    pid_k[p]=pid_k[p] + i;
    pid.tune(pid_i, pid_k.p, pid_k.i, pid_k.d);
end


local acts_set={
    lft=function(k) sol_f(-1); end, 
    rgt=function(k) sol_f( 1); end, 
    lpg=function(k) sol_f(-10); end, 
    rpg=function(k) sol_f( 10); end, 
--    ok=function(k) gui.gotop(); end, 
}


local acts_tune={
    lft=function(p) k_f(p, -0.1); end, 
    rgt=function(p) k_f(p,  0.1); end, 
    lpg=function(p) k_f(p, -1); end, 
    rpg=function(p) k_f(p,  1); end, 
--    ok=function(k) gui.gotop(); end, 
}


local sol_tsk = function()
    if xtm then
	xtm = false;
	cur_t = adc[1];
	pid.inp(pid_i, cur_t);
    else
	xtm = true;

	local acc = pid.out(pid_i);
	--print("acc", acc);
	pwm.DC1 = acc;
	pwm.DC2 = acc;
	--pwm.DC3 = acc;
	pwm.SGN1 = acc;

    end

    return true;
end


local menu_sol={
	{name="Set t", act=acts_set, par=9, ind_t=function(y, k) oled.print(94, y, string.format("%.1f%%", solder_t ) ); end, },
	{name="Curr t", par=1, ind_t=function(y, k) oled.print(94, y, string.format("%.1f%%", cur_t ) ); end, },
	{name="Solder PWM =", par=9, ind_t=function(y, k) oled.print(94, y, string.format("%.1f%%", pwm[k] ) ); end, },

	{name="tune Kp", par="p", act=acts_tune, ind_t=function(y, p) oled.print(94, y, string.format("%.1f", pid_k[p] ) ); end, },
	{name="tune Ki", par="i", act=acts_tune, ind_t=function(y, p) oled.print(94, y, string.format("%.1f", pid_k[p] ) ); end, },
	{name="tune Kd", par="d", act=acts_tune, ind_t=function(y, p) oled.print(94, y, string.format("%.1f", pid_k[p] ) ); end, },

	{name="exit Solder", act={ ok=function() solder_run=nil; pid.del(pid_i); gui.exit(); end, }, },
	{name=" ..redraw"},
	cb=sol_tsk,
}


return menu_sol
