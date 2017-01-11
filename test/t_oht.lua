#!../out/bin/lua

---
-- \file    t_oht.lua
-- \brief   Test for the Ordered Hash Table
local   Test   = require ('t').Test
local   Oht    = require ('t').OrderedHashTable

local tests = {
	setUp = function( self )
		self.keys = { 'one'  , 'two'   , 'three', 'four'  , 'five' , 'six'   , 'seven'   }
		self.vals = { 'first', 'second', 'third', 'fourth', 'fifth', 'sixth' , 'seventh' }
		assert( #self.keys == #self.vals, "Keys and Values must be of same length" )
		self.o    = Oht( )
		self.len  = 0
		-- insertion order is preserved inside the Hash table
		for i = 1, #self.keys do
			self.o[ self.keys[i] ] = self.vals[i] .. ' position'
			self.len  = self.len+1   -- count inserts
		end
		assert( #self.keys == self.len, "Number of inserts must equal length of Keys" )
	end,

	--tearDown = function( self )  -- not necessary for this suite
	--end,

	test_LengthMustEqualInserts = function( self )
		-- #DESC:Length of OrderedHashTable must be equal number of inserts
		assert( #self.o == self.len, "Length of OrderedHashTable must equal number of inserts" )
	end,

	test_AccessIndexedElements = function( self )
		-- #DESC:All elements must be available by their indexes
		for i=1, self.len do
			assert( self.o[i] == self.vals[i] .. ' position', "Indexed value must match original value" )
			assert( self.o[self.keys[i]] == self.o[i], "Hash access must match index access" )
		end
	end,

	test_AddElement = function( self )
		-- #DESC:Adding element increases size
		local o_len = #self.o
		local value   = "eighth position" 

		self.o.eight  = value
		assert( #self.o      == o_len+1,    "Adding element must increase length" )
		assert( self.o.eight == value,      "New Element is available under given key" )
		assert( self.o[ #self.o ] == value, "New Element is available as last element" )
	end,
	
	test_DeleteHashElement = function( self )
		-- #DESC:Removing Hash element decreases size and move down higher keys
		local o_len = #self.o
		local four  = self.o.four
		local six   = self.o.six

		self.o.five   = nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o.four == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o.six  == six,     "Removing element musn't affect higher keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,
	
	test_DeleteIndexElement = function( self )
		-- #DESC:Removing Index element decreases size and move down higher keys
		local o_len = #self.o
		local four  = self.o[4]
		local six   = self.o[6]

		self.o[5]   = nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o.four == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o.six  == six,     "Removing element musn't affect higher keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,

	test_InsertElement = function( self )
		-- #DESC:Insert element increases size and move up higher keys
		local o_len  = #self.o
		local two    = self.o[2]
		local three  = self.o[3]
		local value  = 'oddth position'
		local key    = 'odd'

		Oht.insert( self.o, 3, key, value )
		assert( #self.o      == o_len+1, "Inserting element must increase length" )
		assert( self.o[key]  == value,   "Inserted  element must be available by key" )
		assert( self.o[3]    == value,   "Inserted  element must be available by index" )
		assert( self.o.two   == two,     "Inserting element musn't affect lower keys" )
		assert( self.o[2]    == two,     "Inserting element musn't affect original lower keys" )
		assert( self.o.three == three,   "Inserting element musn't affect original higher keys" )
		assert( self.o[4]    == three,   "Inserting element must move up higher keys" )
	end,

	test_ReplaceHashElement = function( self )
		-- #DESC:Replace Hash element remains size, indexes and keys
		local o_len  = #self.o
		local idx    = 3
		local key    = 'three'
		local val    = 'thirdish'
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ key ] = val .. ' position'
		for i=1,self.len do
			assert( self.o[i]       == vals[i] .. ' position', "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i] .. ' position', "Hashed  Values match after Replace" )
			assert( Oht.getIndex(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	test_ReplaceIndexElement = function( self )
		-- #DESC:Replace Index element remains size, indexes and keys
		local o_len  = #self.o
		local idx    = 3
		local key    = 'three'
		local val    = 'thirdish'
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ idx ] = val .. ' position'
		for i=1,self.len do
			assert( self.o[i]       == vals[i] .. ' position', "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i] .. ' position', "Hashed  Values match after Replace" )
			assert( Oht.getIndex(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	test_CantCreateIndexOutOfBoundElement = function( self )
		-- #DESC:Replace Index element remains size, indexes and keys
		local o_len    = #self.o
		local oob_idx  = o_len + 1
		local val      = "doesn't matter"
		local func     = function() self.o[ oob_idx ] = val .. ' position' end

		assert( not pcall( func ), "Not allowed to create elements with indexes" )
	end,
}

t_oht = Test( tests )
t_oht( )
print( t_oht )
