led = pio.GPIO12

pio.pin.setdir(pio.OUTPUT, led)

while true do
  pio.pin.sethigh(led)
  tmr.delay(500)
  pio.pin.setlow(led)
  tmr.delay(500)
end
