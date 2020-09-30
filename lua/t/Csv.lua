-- \file      lua/t/Csv.lua
-- \brief     CSV?TSV Parser 
-- \detail    This jumps heavily between C and  Lua because it makes plenty use of
--            io:lines() instead of reimplementing that logic.  Main function is
--            csv:read() and csv:write().
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Csv     = require( "t.csv" )
local io_open = io.open

local csv_mt  = debug.getregistry( )[ "T.Csv" ]
local Csv_mt  = getmetatable( Csv )


--csv_mt.__pairs    = function( self ) end

csv_mt.words      = function( self )
	local line = self.handle:read( )  -- current line
	local pos = 1           -- current position in the line
	return function ()      -- iterator function
		while line do           -- repeat while there are lines
			local s, e = line:find(  "%w+", pos )
			if s then            -- found a word?
				pos = e + 1       -- next position is after this word
				return line:sub( s, e)     -- return the word
			else
				line = self.handle:read( )  -- word not found; try next line
				pos = 1           -- restart from first position
			end
		end
		return nil            -- no more lines: end of traversal
	end
end

csv_mt.rows      = function( self )
	local parsed  = { }
	local rowdone = true
	local line    = self.handle:read( )  -- current line
	return function ( )      -- iterator function
		while line do         -- repeat while there are lines
			rowdone = self:parseLine( line, parsed )
			if rowdone then
				local result = parsed
				parsed = { }
				line = self.handle:read( )
				return result
			else
				-- IF the line breaks are not \n we are already in trouble because file:read() strips any kind of line break
				-- this assumes \n, it's the best we can do
				line = line .."\n".. self.handle:read( )
				--line = line .. self.handle:read( 'L' )
			end
		end
		return nil            -- no more lines: end of traversal
	end
end

Csv_mt.__call     = function( csvClass, file, delimiter, quotchar, escapechar, doublequoted )
	local instance        = csvClass.new( )
	instance.delimiter    = delimiter    or ","
	instance.quotchar     = quotchar     or "\""
	instance.escapechar   = escapechar   or "\\"
	instance.doublequoted = nil == doublequoted and false or true
	if 'string' == type( file ) then
		instance.handle = assert( io_open( file, 'r' ) )
	elseif 'userdata' == type( file ) and 'function' == type( file.read ) then
		instance.handle = csv
	else
		error( 'Expected string or file handle' )
	end
	return instance
end

return Csv
