---
-- \file    example/t_ael_popen.lua

local Loop = require('t.Loop')
local Time = require('t.Time')

l = Loop(4)
f = debug.getinfo( 1, "S" ).short_src:match( "^(.*)/" ) .. '/t_ael_popen_load.lua';

l:addTimer( Time( 1000 ), function( )
	print( "Popen() took too long. Stop waiting for result." )
	print( "remove handle", l:removeHandle( h, 'read' ) )
	print( "close handle" )
	-- it doesn't work to just close the handle from this end; it would basically
	-- require to kill the process
	h:close( )
	print( "stop loop" )
	l:stop( )
end )

h = io.popen( 'lua ' .. f )
l:addHandle( h, 'read', function( handle )
	local msg, len = handle:read( '*all' )
	print( msg, len )
	handle:close( )
	l:stop( )
end, h )

l:show( )
l:run( )
