#!../out/bin/lua -i
Loop= require't.Loop'
fmt= string.format
l  = Loop( 10 )
fileName = arg[1] or 't_buf_pck.lua';


function k( )
	local f = io.open( fileName, "r" )
	l:addHandle( f, 'read', function( )
		local d = f:read( 200 )
		if d then
			print( '----------', f, #d, '-----------------' )
			print( d )
		else
			print( f, "end" )
			l:removeHandle( f, 'read' )
			f:close( )
			l:stop( )
		end
	end )
end

k( )

l:show( )
l:run( )
