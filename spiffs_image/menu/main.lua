local stf=dofile "gstuff/menu.lua"

local menu_main = {
	{name="Signal", menu="menu/sgn", ind_t=stf.ind, }, 
	{name="Led", menu="menu/led", ind_t=stf.ind, }, 
	{name="DC", menu="menu/dc", ind_t=stf.ind, }, 
	{name="AC", menu="menu/ac", ind_t=stf.ind, }, 
--	{name="ADC", menu=menu_adc, ind_t=stf.ind, }, 
	{name="DAC", menu="menu/dac", ind_t=stf.ind, }, 
	{name="PWM", menu="menu/pwm", ind_t=stf.ind, }, 
	{name="Gpio", menu="menu/pio", ind_t=stf.ind, },
	{name="..redraw menu"}
};
stf=nil

return menu_main
