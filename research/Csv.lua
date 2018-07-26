#!/usr/bin/lua

local csvLineIterator__call = function( self )
	for i = 1,self.skipLines do
		local line = self.file:read( "*line" )
	end
	return function( )
		-- local line = string.gsub(f:read("*line"), "'", "\\'")
		local line = self.file:read( "*line" )
		-- print (line)
		if line then
			local multiLine = false
			line            = string.gsub( line, self.quotes, "\\"..self.quotes ) .. self.separator -- trailing separator
			local row       = { }                       -- table to collect fields
			local f_start   = 1
			repeat
				-- multiLine field
				if multiLine then
					line = line .. string.gsub( self.file:read( "*line" ), self.quotes, "\\"..self.quotes) .. self.separator
					-- line = line .. f:read("*line") .. ','
					-- print (line)
					local i  = f_start
					repeat
						-- find closing quote; chew accross escaped quotes
						a, i, c = string.find( line, '(\\?)'..self.quotes, i+1 )
					until c ~= '\\'    -- not an escaped quote?
					if i then
						local f = string.sub( line, f_start+1, i-1 )
						table.insert( row, string.gsub( f, '\\'..self.quotes, self.quotes ) )
						f_start = string.find(line, self.separator, i) + 1
						multiLine = false
					end
				end

				-- next field is quoted? (start with `"'?)
				if string.find( line, '^'..self.quotes, f_start ) then
					local a, c
					local i  = f_start
					repeat
						-- find closing quote; chew accross escaped quotes
						a, i, c = string.find( line, '(\\?)'..self.quotes, i+1 )
					until c ~= '\\'    -- not an escaped quote?
					if not i then
						-- error('unmatched "')
						line = string.gsub( line, '\\'..self.separator..'$', '\n' )
						multiLine = true
					else
						local f = string.sub( line, f_start+1, i-1 )
						table.insert( row, string.gsub( f, '\\'..self.quotes, self.quotes ) )
						f_start = string.find( line, self.separator, i ) + 1
					end
				else                 -- unquoted; find next separator
					local nexti = string.find( line, self.separator, f_start )
					table.insert(row, string.sub( line, f_start, nexti-1 ))
					f_start = nexti + 1
				end
			until f_start > string.len( line )

			return row
		else
			self.file:close( )
		end
	end
end

local _tostring = function( self )
	return string.format( "T.Cvs[%s]", self.fileName )
end

local constructor = function( self, fileName, separator, quotes, skipLines )
	local instance     = { }
	instance.fileName  = fileName
	instance.file      = assert( io.open( instance.fileName, 'r' ) )
	instance.separator = separator or ','
	instance.quotes    = quotes    or '"'
	instance.skipLines = skipLines or 0
	return setmetatable(
		  instance
		, {
			  __call     = csvLineIterator__call
			, __tostring = _tostring
		} )
end

return setmetatable( {}, {
	__call = constructor
} )
