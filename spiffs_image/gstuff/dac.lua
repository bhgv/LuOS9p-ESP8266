
return {
  acts={
    lft=function() adc.DAC=adc.DAC-1 end, 
    rgt=function() adc.DAC=adc.DAC+1 end, 
    lpg=function() adc.DAC=adc.DAC-10 end, 
    rpg=function() adc.DAC=adc.DAC+10 end, 
    ok=function() gui.gotop(); end, 
  },

  ind=function(y, k) 
    oled.print(94, y, string.format("%.1f%%", adc.DAC ) ); 
  end
}

