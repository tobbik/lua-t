-- \file      lua/t/Csv.lua
-- \brief     CSV/TSV Parser
-- \detail    This jumps heavily between C and  Lua because it makes plenty use of
--            io:read() instead of re-implementing that logic.  Main function is
--            csv:rows() and csv:write().
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Csv     = require( "t.csv" )
local Buffer  = require( "t.Buffer" )

local csv_mt  = debug.getregistry( )[ "T.Csv" ]
local Csv_mt  = getmetatable( Csv )

local csv_new = Csv.new
Csv.new       = nil

local use_headers = function( self, result )
	for i,v in ipairs(self.headers) do
		result[ v ] = result[ i ]
	end
end

csv_mt.rows = function( self, source )
	assert( 'function' == type(source), "Argument must be an iterator function" )
	local records,parsed,rowdone,line = 0 ,{ }, true, source( )  -- current line
	if 'boolean' == type(self.headers) and self.headers then
		self.headers = { }
		rowdone = self:parseLine( line, self.headers )
		line    = source( )
	end
	return function ( )      -- iterator function
		while line do         -- repeat while there are lines
			print( Buffer(line):toHex())
			--print(("LINE:  _%s_"):format(line))
			rowdone = self:parseLine( line, parsed )
			if 0==#line then
				line = source( )
			elseif rowdone then
				local result = parsed
				parsed,line,records = { }, source( ), records+1
				return self.headers and use_headers(self,result) or result, records
			else
				-- IF the line breaks are not \n we are already in trouble because file:read() strips any kind of line break
				-- this assumes \n, it's the best we can do. TODO: smarter detect line breaks
				line = line .."\n".. source( )
			end
		end
		return nil            -- no more lines: end of traversal
	end
end

Csv_mt.__call = function( csvClass, delimiter, headers, quotchar, escapechar, doublequoted )
	local csv  = csv_new( )
	--delimiter = 'table' = type(delimiter) and delimiter.delimiter or delimiter
	if 'table' == type( delimiter ) then
		csv.delimiter    = delimiter.delimiter    or ","
		csv.headers      = nil ~= delimiter.headers      and delimiter.headers or false      -- default to false
		csv.quotchar     = delimiter.quotchar     or "\""
		csv.escapechar   = delimiter.escapechar   or "\\"
		csv.doublequoted = nil == delimiter.doublequoted and true or delimiter.doublequoted  -- default to true
	else
		csv.delimiter    = delimiter              or ","
		csv.headers      = nil ~= headers      and headers or false      -- default to false
		csv.quotchar     = quotchar               or "\""
		csv.escapechar   = escapechar             or "\\"
		csv.doublequoted = nil == doublequoted and true or doublequoted  -- default to true
	end
	return csv
end

return Csv
