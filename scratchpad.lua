Socket  = require't.Net.Socket'
Address = require't.Net.Address'
Family  = require't.Net.Family'
Loop    = require't.Loop'
Interface = require't.Net.Interface'
tbl = require't.Table'
pp,t_equals = tbl.pprint, tbl.equals
snd_buffer_max = 1048576 -- according to manpage

n = {
	host  = Interface.default( ).address.ip,
	port  = 1234,
	loop  = Loop()
}
n.srvSck, n.srvAdr = Socket.listen( n.host, n.port )
print( "X.SRVR:", n.srvSck, n.srvAdr )

-- accept server for each test and set up recv()
local makeReceiver = function( s, receiver )
	local acpt = function( x )
		x.rcvSck, x.rcvAdr = x.srvSck:accept( )
		print("X.RECV:",x.rcvSck, x.rcvAdr)
		x.loop:addHandle( x.rcvSck, "read", receiver, x )
		x.loop:removeHandle( x.srvSck, "read" )
	end
	s.loop:addHandle( s.srvSck, "read", acpt, s )
end

local prepareSender = function( s, msg )
	s.sndSck, s.sndAdr = Socket.connect( s.srvAdr )
	print("X.SEND:", s.sndSck, s.sndAdr)
	if s.sndSck.sendbuffer < snd_buffer_max then
		print("ADJUSTING sendbuffer:", s.sndSck.sendbuffer)
		s.sndSck.sendbuffer = snd_buffer_max
	end
	return msg:rep( s.sndSck.sendbuffer // #msg - 900)
end


-- for this test we use blocking sending only.  The test is for recv(); no need
-- to complicate it
local makeSender = function( s, payload )
	local f = function( x,p )
		print("SENDING:", s.sndSck.sendbuffer, #p)
		local snt = x.sndSck:send( p )
		print("SENT:", snt)
		assert( snt == #p, ("Should have sent all(%d bytes) but sent %d bytes"):format( #p, snt ) )
		x.loop:removeHandle( x.sndSck, "write" )
		x.sndSck:shutdown( "wr" )
	end
	s.loop:addHandle( s.sndSck, 'wr', f, s, payload )
	s.loop:show( )
	s.loop:run( )
end


payload  = prepareSender( n, "TestMessage content for recieving full string -- " )
cnt      = 0
receiver = function( x )
	local msg,len = x.rcvSck:recv( )
	assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
	if msg then
		assert( type(msg)=='string', ("Expected `%s` but got `%s`"):format( 'string', type(msg) ) )
		assert( msg == payload:sub(cnt+1,cnt+len), ("Message was %s but should have been %s"):format(
		          msg, payload:sub(cnt+1,cnt+len) ) )
		cnt = cnt+len
		print("CNT:", cnt, len)
	else
		assert( cnt==#payload, ("Expected %d but got %d bytes"):format( #payload, cnt) )
		x.loop:show()
		--x.loop:removeHandle( x.rcvSck, "read")
		x.loop:show()
		x.loop:clean()
		x.sndSck:close( )
		x.rcvSck:close( )
		x.srvSck:close( )
	end
end
makeReceiver( n, receiver )
makeSender( n, payload )
