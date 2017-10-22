return {
  acts={
    lft=function(k)  end, 
    rgt=function(k)  end, 
    lpg=function(k)  end, 
    rpg=function(k)  end, 
    ok=function(k) gui.gotop(); end, 
  },

  ind=function(y, k) 
    oled.print(94, y, string.format("%.1f%%", adc[k] ) ); 
  end
}
