return {
  acts={
    lft=function(k) pwm[k]=pwm[k]-1 end, 
    rgt=function(k) pwm[k]=pwm[k]+1 end, 
    lpg=function(k) pwm[k]=pwm[k]-10 end, 
    rpg=function(k) pwm[k]=pwm[k]+10 end, 
    ok=function(k) gui.gotop(); end, 
  },

  ind=function(y, k) 
    oled.print(94, y, string.format("%.1f%%", pwm[k] ) ); 
  end
}
