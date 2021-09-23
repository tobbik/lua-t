Loop      = require't.Loop'

l = Loop()

runs,delta, dev  = 200, 0, {}
success     = function( n, s, d )
  local ms_passed = Loop:time() - s
  print( n, d, ms_passed, ms_passed-d )
  delta = delta + (ms_passed-d)
  table.insert(dev, ms_passed-d)
end
start = Loop.time()

for n=1,runs do
  delay = math.random( 8*n, 20*n)
  l:addTask( delay, success, n, start, delay )
end
l:run( )
mean, vari = delta/runs, 0
for i,d in ipairs(dev) do
  vari = vari + ((d-mean) ^ 2)
end
vari = math.sqrt( vari/runs )

print( ("Average delta over %d runs was %fms with standard deviation %f"):format( runs, delta/runs, vari ) )
