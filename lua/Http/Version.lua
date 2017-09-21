local versions =  {
	  [ 0 ]   = "ILLEGAL"
	, [ 1 ]   = "HTTP/0.9"
	, [ 2 ]   = "HTTP/1.0"
	, [ 3 ]   = "HTTP/1.1"
	, [ 4 ]   = "HTTP/2"
}
local reverse = { }

for k,v in pairs( versions ) do
	reverse[ v ] = k
end

for k,v in pairs( reverse ) do
	versions[ k ] = v
end



return versions
