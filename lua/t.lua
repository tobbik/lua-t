local core = require( "t.core" )
local case = require( "t.Test.Case" )


return {
	require          = core.require,
	type             = core.type,
	proxyTableIndex  = core.proxyTableIndex,
}
