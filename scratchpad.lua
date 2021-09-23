Loop      = require't.Loop'

l = Loop()

arg1,arg2  = 100, 'this is a string'
time,start = math.random(80,600), Loop.time()
print(("Requesting %dms"):format(time))
success    = function( a, b )
	local ms_passed = Loop:time() - start
	print( a, arg1 )
	print( b, arg2 )
	print( delay, ms_passed )
end

l:addTask( time, success, arg1, arg2 )
--loop:run()
