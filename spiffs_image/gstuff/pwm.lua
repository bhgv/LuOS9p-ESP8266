return {
  acts={
    lft=function(k) pwm[k]=pwm[k]-1 end, 
    rgt=function(k) pwm[k]=pwm[k]+1 end, 
    lpg=function(k) pwm[k]=pwm[k]-10 end, 
    rpg=function(k) pwm[k]=pwm[k]+10 end, 
    ok=function(k) top_menu(); end, 
  },

  ind=function(d, y, k) 
    d.print(94, y, string.format("%.1f%%", pwm[k] ) ); 
  end
}
