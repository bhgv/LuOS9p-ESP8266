local stf=dofile "gstuff/adc.lua"

local menu_pwm={
	{name="ADC 0", act=stf.acts, par=0, ind_t=stf.ind },
	{name="ADC 1", act=stf.acts, par=1, ind_t=stf.ind },
	{name="ADC 2 (Light)", act=stf.acts, par=2, ind_t=stf.ind },
	{name="ADC 3 (Iled)", act=stf.acts, par=3, ind_t=stf.ind },
--	{name="PWM 4", act=stf.acts, par=4, ind_t=stf.ind },
	{name=" ..main menu"}
}
stf=nil

return menu_pwm
