-- \file      lua/t/Csv.lua
-- \brief     CSV/TSV Parser
-- \detail    This jumps heavily between C and  Lua because it makes plenty use of
--            io:read() instead of re-implementing that logic.  Main function is
--            csv:rows() and csv:write().
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Csv     = require( "t.csv" )
local Buffer  = require( "t.Buffer" )

local _mt

local use_headers = function( self, result )
	for i,v in ipairs(self.headers) do
		result[ v ] = result[ i ]
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Csv",
	__tostring = function( self )
		return ("t.Csv[%s:%s:%s:%s]"):format(
			self.delimiter == "\t" and "<TAB>" or self.delimiter,
			self.escapechar,
			self.quotchar,
			self.doublequoted and "true" or "false" )
	end,
	-- build in functions
	rows       = function( self, source )
		assert( 'function' == type(source), "Argument must be an iterator function" )
		local records,parsed,rowdone,line,field, fld_cnt = 0 ,{ }, true, source( ), nil, 0
		if 'boolean' == type(self.headers) and self.headers then
			self.headers = { }
			rowdone,field,fld_cnt = Csv.parseLine( self.delimiter, self.quotchar, self.escapechar, self.doublequoted, line..'\n', self.headers, fld_cnt )
			line    = source( )
		end
		return function( )       -- iterator function
			while line do         -- repeat while there are lines
				local buff =  Buffer(line)
				--print(("%d BUFF: %s"):format(#buff, buff:toHex() ))
				--print(("%d LINE: _%s_"):format( #line, line))
				rowdone,field,fld_cnt = Csv.parseLine( self.delimiter, self.quotchar, self.escapechar, self.doublequoted, line ..'\n', parsed, rowdone and 0 or fld_cnt )
				--print(("FIELD: _%s_   LINE: _%s_"):format(field, line))
				if 0==#line then
					line = source( )
				elseif rowdone then
					local result = parsed
					parsed,line,records = { }, source( ), records+1
					return self.headers and use_headers( self,result)  or result, records
				else
					-- IF the line breaks are not \n we are already in trouble because file:read() strips any kind of line break
					-- this assumes \n, it's the best we can do. TODO: smarter detect line breaks
					line = field .. source( )
				end
			end
			return nil            -- no more lines: end of traversal
		end
	end
}
_mt.__index = _mt

return setmetatable( {
	_VERSION     = 't.Csv 0.1.0',
	_DESCRIPTION = 'lua-t Csv processing.',
	_URL         = 'https://gitlab.com/tobbik/lua-t',
	_LICENSE     = 'MIT',
	split        = Csv.split
}, {
	__call   = function( csvClass, delimiter, headers, quotchar, escapechar, doublequoted )
		local csv  = { }
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
		return setmetatable( csv, _mt )
	end
} )

