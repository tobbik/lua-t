-- \file      lua/t/Csv.lua
-- \brief     CSV/TSV Parser
-- \detail    This jumps heavily between C and  Lua because it makes plenty use of
--            io:read() instead of re-implementing that logic.  Main function is
--            csv:rows() and csv:write().
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Csv     = require( "t.csv" )
local io_open = io.open

local csv_mt  = debug.getregistry( )[ "T.Csv" ]
local Csv_mt  = getmetatable( Csv )

local csv_new = Csv.new
Csv.new       = nil
--csv_mt.__pairs    = function( self ) end

csv_mt.rows      = function( self )
	local parsed,rowdone,line  = { }, true, self.handle:read( )  -- current line
	return function ( )      -- iterator function
		while line do         -- repeat while there are lines
			rowdone = self:parseLine( line, parsed )
			if rowdone then
				local result = parsed
				parsed,line = { }, self.handle:read( )
				return result, self.records
			else
				-- IF the line breaks are not \n we are already in trouble because file:read() strips any kind of line break
				-- this assumes \n, it's the best we can do. TODO: smarter detect line breaks
				line = line .."\n".. self.handle:read( )
				--line = line .. self.handle:read( 'L' )
			end
		end
		return nil            -- no more lines: end of traversal
	end
end

Csv_mt.__call     = function( csvClass, file, delimiter, quotchar, escapechar, doublequoted )
	local instance        = csv_new( )
	local fHandle
	if 'table' == type( file ) then
		instance.delimiter    = file.delimiter    or ","
		instance.quotchar     = file.quotchar     or "\""
		instance.escapechar   = file.escapechar   or "\\"
		instance.doublequoted = nil == file.doublequoted and false or true
		fHandle               = file.handle
	else
		instance.delimiter    = delimiter    or ","
		instance.quotchar     = quotchar     or "\""
		instance.escapechar   = escapechar   or "\\"
		instance.doublequoted = nil == doublequoted and false or true
		fHandle               = file
	end
	if 'string' == type( fHandle ) then
		instance.handle = assert( io_open( fHandle, 'r' ) )
	elseif 'userdata' == type( fHandle ) and 'function' == type( fHandle.read ) then
		instance.handle = fHandle
	else
		error( 'Expected string or Lua file handle' )
	end
	return instance
end

return Csv
