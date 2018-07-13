local t_assert, t_type = require't'.assert,require't'.type
local Pack             = require't.Pack'
local s_format         = string.format

local astFmt = function( cnd, ... )
	if cnd then
		return true
	else
		return false, s_format( ... )
	end
end

return {
	-- #########################################################################
	-- assert helpers
	-- this functions are meant to be fed into assert.  Having assert() being
	-- called in the test-case itself has the advantage that it throws the
	-- exception right here which in turn allows the traceback to report the
	-- proper location of the error better.
	Sck = function( sck, pro, fam, typ )
		local x,m
		x,m = astFmt( 'T.Net.Socket' == t_type( sck ), "Should be `t.Net.Socket` but was `%s`", t_type( sck ) )
		if not x then return x,m end
		x,m = astFmt( pro == sck.protocol, "Protocol should be `%s`but is `%s`", pro, sck.protocol )
		if not x then return x,m end
		x,m = astFmt( fam == sck.family  , "Family should be `%s` but is `%s`", fam, sck.family )
		if not x then return x,m end
		x,m = astFmt( typ == sck.type    , "Type should be `%s` but is `%s`", typ, sck.type )
		if not x then return x,m else return x end
	end,

	Adr = function( adr, family, ip, port )
		local x,m
		x,m = astFmt( 'T.Net.Address' == t_type( adr ), "Should be `t.Net.Address` but was `%s`", t_type( adr ) )
		if not x then return x,m end
		x,m = astFmt( ip     == adr.ip,    "IP should be `%s` but is `%s`",   ip,     adr.ip )
		if not x then return x,m end
		x,m =astFmt( family == adr.family, "Family should be `%s` but is `%s`", family, adr.family )
		if not x then return x,m end
		if type( port ) ~= 'number' then  -- such as 'any'
			x,m = astFmt( 0 ~= port       ,    "Port should be `%s` but is %s", port, adr.port )
		else
			x,m = astFmt( port == adr.port,    "Port should be `%s` but is %s", port, adr.port )
		end
		if not x then
			return x,m
		else
			return x
		end
	end,


	Packer = function( pck, typ, sizByte, sizBit )
		local expect   = 't.Pack.' .. typ
		local pck_name = tostring( pck ):match( '(.*):' )
		--print( 'PCK_NAME:', expect, pck_name )
		t_assert( expect == pck_name,
		          "Packer should be '%s' but was '%s'", expect, pck_name )
		local sByt,sBit = Pack.getSize( pck )
		t_assert( sByt == sizByte, "Expected Byte size %d was %d for `%s`", sizByte, sByt, typ )
		t_assert( sBit == sizBit,  "Expected Bit  size %d was %d for `%s`", sizByte, sBit, typ )
	end
}
