local Table = require.t.Table

local versions =  {
	  [ 0 ]   = "ILLEGAL"
	, [ 1 ]   = "HTTP/0.9"
	, [ 2 ]   = "HTTP/1.0"
	, [ 3 ]   = "HTTP/1.1"
	, [ 4 ]   = "HTTP/2"
}

-- reverse lookup
for k,v in pairs( Table.clone(versions) ) do
	versions[ v ] = k
end

return versions
