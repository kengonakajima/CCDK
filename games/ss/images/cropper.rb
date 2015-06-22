32.times do |x|
 42.times do |y|
   ind = x + y * 32
   cmd = "convert -crop 24x24+#{x*24}+#{y*24} ssbase1024.png crop_#{ind}.png"
   system(cmd)
 end
end
