#!../out/bin/lua -i
t  = require't'
fmt= string.format
l  = t.Loop(10)
fileName = 'buf.lua';


function k ()
	local f = io.open( fileName, "r" )
	l:addHandle( f, true, function( )
		local d = f:read( 200 )
		if d then
			print( f, #d )
		else
			print( f, "end" )
			l:removeHandle( f, true )
			f:close()
			l:stop()
		end
	end )
end

k(  )

l:show()
l:run()
