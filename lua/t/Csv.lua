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

csv_mt.__len      = function( self )
	local count = 1
	while true do
		if self.handle:read( ) then
			count = count+1
		else
			self.handle:seek( 'set' )
			return count
		end
	end
end

csv_mt.__gc       = function( self ) self.handle:close( ) end

csv_mt.__pairs    = function( self ) end

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
	local line = self.handle:read( )  -- current line
	return function ( )      -- iterator function
		while line do         -- repeat while there are lines
			local parsed = { }
			self:parse( line, self.state, parsed )
			line = self.handle:read( )
			return parsed
		end
		return nil            -- no more lines: end of traversal
	end
end

csv_mt.lines      = function( self )
	local line = self.handle:read( )  -- current line
	return function ( )      -- iterator function
		while line do         -- repeat while there are lines
			local parsed = { }
			self:p( line, self.state, parsed )
			line = self.handle:read( )
			return parsed
		end
		return nil            -- no more lines: end of traversal
	end
end

csv_mt.__index    = csv_mt

Csv_mt.__call     = function( self, csv, delimter, quotchar, escapechar, doublequoted )
	local handle
	if 'string' == type(csv) then
		handle = assert( io_open( csv, 'r' ) )
	elseif 'userdata' == type(csv) and 'function' == type(csv.read) then
		handle = csv
	else
		error( 'Expected string or file handle' )
	end
	local instance = setmetatable( {
			delimiter    = delimter     or ",",
			quotchar     = quotchar     or "\"",
			escapechar   = escapechar   or "\\",
			doublequoted = nil == doublequoted and false or true,
			handle       = handle
		}, csv_mt )
	instance.state = instance:build( instance.delimiter, instance.quotchar, instance.escapechar, instance.doublequoted )
	return instance
end

return Csv
