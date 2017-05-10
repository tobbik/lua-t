local T, Test, Time, format = require"t", require"t.Test", require"t.Time", string.format

local m = {
	"t_ael",
	"t_buf"                 , "t_buf_seg",
	"t_net_sck_create"      , "t_net_sck_bind",
	"t_net_sck_connect"     , "t_net_sck_listen",
	"t_net_sck_dgram_recv"  , "t_net_sck_dgram_send",
	"t_net_sck_stream_recv" , "t_net_sck_stream_send",
	"t_oht"                 , "t_set",
	"t_t"                   , "t_t_equals",
	"t_tbl",
	"t_tst",
}


local run = function( do_pat, no_pat )
	local do_pat, no_pat = do_pat                   or ''    , no_pat                   or '^$'
	local td_pat, tn_pat = do_pat:match( ':(.*)$' ) or ''    , no_pat:match( ':(.*)$' ) or '^$'
	local do_pat, no_pat = do_pat:match( '^(.*):' ) or do_pat, no_pat:match( '^(.*):' ) or no_pat
	local ctx   = Test.Context( td_pat, tn_pat, nil, nil, nil, function() end )
	local total = {
		count   = 0,
		pass    = 0,
		skip    = 0,
		todo    = 0,
		time    = Time( 1 ) - Time( 1 ),
		allTime = Time( )
	}
	for k,v in pairs( m ) do
		--local runit =  v:match( do_pat ) and not v:match( no_pat )
		--print('------', runit,  do_pat, no_pat, v:match( do_pat ), v:match( no_pat ), v )
		if v:match( do_pat ) and not v:match( no_pat ) then
			print( format( "--------- EXECUTING: %s   ---------", v ) )
			local c_test = T.require( v )
			if not c_test( ctx ) then
				t = c_test --> push test suite into global scope
				break
			end
			local metrics = ctx:getMetrics( c_test )
			total.count = total.count + metrics.count
			total.pass  = total.pass  + metrics.pass
			total.skip  = total.skip  + metrics.skip 
			total.todo  = total.todo  + metrics.todo
			total.time  = total.time  + metrics.time
			print( format( "--------- Done in : %.3f s" , ctx:getMetrics( c_test ).time:get()/1000 ) )
		end
	end
	total.allTime:since( )
	print( format( "Executed %d tests in %.3f seconds", total.count, total.allTime:get()/1000 ))
end

local include_pattern = ""
local exclude_pattern = "^$"
if arg[ 1 ] then
	include_pattern = tostring( arg[ 1 ] )
end
if arg[ 2 ] then
	exclude_pattern = arg[ 2 ]
end

run( include_pattern, exclude_pattern )
