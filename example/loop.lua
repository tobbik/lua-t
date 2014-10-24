xt=require'xt'


f = function(func, ...)
	local args = {...}
	return function()
		return func( table.unpack( args ) )
	end
end

l   = xt.Loop( 40 )
tm1 = xt.Time( 2000 )
tm2 = xt.Time( 2000 )
l:addTimer( tm1 , false, print, 1,2,3,4,5,6,7 )
l:addTimer( tm2 , false, f(print,8,7,6,5,4,3,2,1) )
l:run()
