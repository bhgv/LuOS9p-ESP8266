return {
  acts={
    lft=function(k)  end, 
    rgt=function(k)  end, 
    lpg=function(k)  end, 
    rpg=function(k)  end, 
    ok=function(k) top_menu(); end, 
  },

  ind=function(d, y, k) 
    d.print(94, y, string.format("%.1f%%", adc[k] ) ); 
  end
}
