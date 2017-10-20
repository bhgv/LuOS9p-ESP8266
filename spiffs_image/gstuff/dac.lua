
return {
  acts={
    lft=function() adc.DAC=adc.DAC-1 end, 
    rgt=function() adc.DAC=adc.DAC+1 end, 
    lpg=function() adc.DAC=adc.DAC-10 end, 
    rpg=function() adc.DAC=adc.DAC+10 end, 
  },

  ind=function(d, y, k) 
    d.print(100, y, string.format("%d%%", math.ceil(adc.DAC) ) ); 
  end
}

