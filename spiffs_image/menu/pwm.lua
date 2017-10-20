local stf=dofile "gstuff/pwm.lua"

local menu_pwm={
	{name="PWM 0", act=stf.acts, par=0, ind_t=stf.ind },
	{name="PWM 1", act=stf.acts, par=1, ind_t=stf.ind },
	{name="PWM 2", act=stf.acts, par=2, ind_t=stf.ind },
	{name="PWM 3", act=stf.acts, par=3, ind_t=stf.ind },
	{name="PWM 4", act=stf.acts, par=4, ind_t=stf.ind },
	{name="..to main menu"}
}
stf=nil

return menu_pwm
