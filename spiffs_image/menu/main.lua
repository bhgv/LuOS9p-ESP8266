local stf=dofile "gstuff/menu.lua"

local menu_main = {
	{name="Sign led", menu="menu/sgn", ind_t=stf.ind, }, 
	{name="Led pwr", menu="menu/led", ind_t=stf.ind, }, 
	{name="DC out", menu="menu/dc", ind_t=stf.ind, }, 
	{name="AC key", menu="menu/ac", ind_t=stf.ind, }, 
	{name="ADC in", menu="menu/adc", ind_t=stf.ind, }, 
	{name="DAC out", menu="menu/dac", ind_t=stf.ind, }, 
	{name="PWM out", menu="menu/pwm", ind_t=stf.ind, }, 
	{name="Gpio in/out", menu="menu/pio", ind_t=stf.ind, },
	{name=" ..redraw"}
};
stf=nil

return menu_main
