local T = require( 't' )

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

local run = function( m_pat, m_inv, t_pat, t_inv )
	local m_pat = m_pat or ''
	for k,v in pairs( m ) do
		if (not m_inv and v:match( m_pat )) or (m_inv and not v:match( m_pat )) then
			local test = T.require( v )
			if     t_pat and     t_inv then test( t_pat, t_inv )
			elseif t_pat and not t_inv then test( t_pat )
			else                            test( )
			end
		end
	end
end

local module_pattern        = ''
local module_pattern_invert = false
local test_pattern          = ''
local test_pattern_invert   = false
if arg[ 1 ] then
	module_pattern = tostring( arg[ 1 ] )
end
if arg[2] and arg[2]:match( '[tT][rR][uU][eE]' ) or 1==tonumber( arg[2] ) then
	module_pattern_invert = true
end
if arg[ 3 ] then
	test_pattern = tostring( arg[ 3 ] )
end
if arg[4] and arg[4]:match( '[tT][rR][uU][eE]' ) or 1==tonumber( arg[4] ) then
	test_pattern_invert = true
end

run( module_pattern, module_pattern_invert, test_pattern, test_pattern_invert )
