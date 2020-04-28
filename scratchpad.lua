Time=require't.Time'
Loop=require't.Loop'


times = function(l,x)
	local top = 10
	local cb = function( )
		top = top - 1
		print("Loop: " .. x, top)
		if top>0 then
			return Time(math.random(400,1400))
		else
			return top
		end
	end
	l:addTimer(Time(math.random(200,1000)), cb)
end

function Sync(fn)
	local function make_resumer(co)
		return function(...)
			return assert(coroutine.resume(co, ...))
		end
	end
	local co = coroutine.create(fn)
	assert( coroutine.resume(co, make_resumer(co), coroutine.yield ) )
end

to = function(n,fn)
	l:addTimer( Time(n), fn, 'We done here...' )
end

l=Loop(5)

times(l,1)
times(l,2)
l:run()
