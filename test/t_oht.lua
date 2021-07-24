---
-- \file    t_oht.lua
-- \brief   Test for the Ordered Hash Table
local Rtvg   = require( "t" ).require( "rtvg" )
local Table  = require( "t.Table" )
local Test   = require( "t.Test" )
local Oht    = require( "t.OrderedHashTable" )

return {
	len        = 5000,
	beforeEach = function( self )
		self.rtvg   = Rtvg( )
		-- rtvg.getVals() guarantees disjoint arrays!
		self.keys   = self.rtvg:getKeys( self.len )
		self.vals   = self.rtvg:getVals( self.len )
		self.o, len = Oht( ), 0
		-- insertion order is preserved inside the Hash table
		for i = 1, #self.keys do
			self.o[ self.keys[i] ] = self.vals[i]
			len  = len+1   -- count inserts
		end
		assert( #self.keys == #self.vals, "Keys and Values must be of same length" )
		assert( len        == self.len,   "Number of inserts must equal length of Keys" )
	end,

	-- Test cases

	LengthMustEqualInserts = function( self )
		Test.describe( "Length of OrderedHashTable must be equal number of inserts" )
		assert( #self.o == self.len, "Length must equal number of inserts" )
	end,

	-- CONSTRUCTOR TESTS
	TableStyleConstructor = function( self )
		Test.describe( "Oht({a=1},{b=2},{c=3},...) creates proper Oht instance" )
		local o = Oht(
			  { [self.keys[1]] = self.vals[1] }
			, { [self.keys[2]] = self.vals[2] }
			, { [self.keys[3]] = self.vals[3] }
			, { [self.keys[4]] = self.vals[4] }
			, { [self.keys[5]] = self.vals[5] }
			, { [self.keys[6]] = self.vals[6] }
			, { [self.keys[7]] = self.vals[7] }
			, { [self.keys[8]] = self.vals[8] }
			, { [self.keys[9]] = self.vals[9] }
		)
		assert( #o == 9, "Length must equal number of constructor arguments" )
	end,

	MixedConstructor = function( self )
		Test.describe( "0=Oht({a=1},{b=2},{c=3},...) and o.d=4 creates proper Oht instance" )
		local o = Oht(
			  { [self.keys[1]] = self.vals[1] }
			, { [self.keys[2]] = self.vals[2] }
			, { [self.keys[3]] = self.vals[3] }
			, { [self.keys[4]] = self.vals[4] }
		)
		o[ self.keys[5] ] = self.vals[5]
		o[ self.keys[6] ] = self.vals[6]
		o[ self.keys[7] ] = self.vals[7]
		assert( #o == 4 + 3, "Length of OrderedHashTable must equal number of arguments" ..
		                     " in construtor plus number of inserts" )
	end,

	CopyConstructor = function( self )
		Test.describe( "OrderedHashTable constructed by copy must match original" )
		local o = Oht( self.o )
		assert( #o == self.len, "Original and clone length must be equal" )
		for i=1, self.len do
			assert( self.o[i] == o[i] , "Indexed value in clone must match original value" )
		end
		for k,v in pairs(o) do
			assert( v == self.o[k], "Keyed value in clone must match original value" )
		end
	end,

	EmptyConstructor = function( self )
		Test.describe( "OrderedHashTable constructed from empty must match original" )
		local o = Oht( )
		assert( #o == 0, "Empty OrderedHashTable should have length 0" )
		for k,v,_ in pairs( self.o ) do  -- use pairs() iterator to fill copy
			o[k] = v
		end

		assert( #self.o == self.len, "Original and clone length must be equal" )
		for i=1, self.len do
			assert( self.o[i] == o[i] , "Indexed value in clone must match original value" )
		end
	end,

	proxyTableByIndexIsSet = function( self )
		Test.describe( "An OrderedHashTable proxyTable is indexed by t.Table.proxyTableIndex" )
		local oht = Oht(
			  { foo1 = "bar1" }
			, { foo2 = "bar2" }
			, { foo3 = "bar3" }
			)
		local prx = oht[ Table.proxyTableIndex ]
		assert( Table.count( prx ) == 6, ("proxyTable should have 3 members but had %d"):format( Table.count(prx) ) )
	end,

	PairsIterator = function( self )
		Test.describe( "Function pair() must iterate in proper order" )
		local ri = 0
		for k,v,i in pairs( self.o ) do
			ri = ri+1
			assert( i == ri, ("Iterator index %d must match running index %d"):format( i, ri) )
			assert( k == self.keys[i], "Iterator hash must match running index hash" )
			--assert( v == self.vals[i], "Iterator value must match running index value" )
		end
		assert( #self.o == ri,
			 "Number of iterations in pairs() ("..ri..
			 ") must be equal to length of OrderedHashTable("..#self.o..")" )
	end,

	IpairsIterator = function( self )
		Test.describe( "Function ipair() must iterate in proper order" )
		local ri = 0
		for i,v,k in ipairs( self.o ) do
			ri = ri+1
			assert( i == ri, ("Iterator index %d must match running index %d"):format( i, ri) )
			if Oht._isCompat then
				assert( k == self.keys[i], ("Iterator hash[%s] must match running index hash[%s]"):format( k, self.keys[i] ) )
			end
			--assert( v == self.vals[i], "Iterator value[%s] must match running index value[%s]", v, self.vals[i] ) )
		end
		assert( #self.o == ri,
			 "Number of iterations in ipairs() ("..ri..
			 ") must be equal to length of OrderedHashTable("..#self.o..")" )
	end,

	ConcatStrings = function( self )
		Test.describe( "Concat concatenates strings" )
		local sep = 'willy nilly'
		local keys = { 'one'  , 'two'   , 'three', 'four'  , 'five' , 'six'   , 'seven'   }
		local vals = { 'first', 'second', 'third', 'fourth', 'fifth', 'sixth' , 'seventh' }
		local o   = Oht()
		for i=1,#keys do o[keys[i]] = vals[i] end

		assert( Oht.concat( o, sep ) == table.concat( vals, sep ),
			 "Concatenated values:\n".. Oht.concat( o, sep ) ..
			 "\n must equal normal table concat results:\n" .. table.concat( vals, sep ) )
		assert( Oht.concat( o, sep, 2, 7 ) == table.concat( vals, sep, 2, 7 ),
			 "Concatenated values:\n".. Oht.concat( o, sep, 2, 7 ) ..
			 "\n must equal normal table concat results:\n" .. table.concat( vals, sep, 2, 7 ) )
	end,

	AccessIndexedElements = function( self )
		Test.describe( "All elements must be available by their indexes" )
		for i=1, self.len do
			assert( self.o[i] == self.vals[i], "Indexed value must match original value" )
			assert( self.o[self.keys[i]] == self.o[i], "Hash access must match index access" )
			assert( Oht.index( self.o, self.keys[i] ) == i, "Key index must match index" )
			assert( Oht.key( self.o, i ) == self.keys[i], "Key index must match index" )
		end
	end,

	AddElement = function( self )
		Test.describe( "Adding element increases size" )
		local key   = self.rtvg:getKey()
		local value = self.rtvg:getVal()

		self.o[ key ]         =  value
		assert( #self.o       == self.len+1, "Adding element must increase length" )
		assert( self.o[ key ] == value,      "New Element is available under given key" )
		assert( self.o[ #self.o ] == value,  "New Element is available as last element" )
	end,

	DeleteHashElement = function( self )
		Test.describe( "Removing Hash element decreases size and move down higher keys" )
		local idx   = math.ceil( self.len / 2 )
		local key   = self.keys[ idx   ]
		local klt   = self.keys[ idx-1 ]
		local kgt   = self.keys[ idx+1 ]
		local vlt   = self.o[ klt ]
		local vgt   = self.o[ kgt ]

		assert( self.o[idx]   ~= nil,     "Element must exist before removing it" )
		self.o[ key ]         =  nil
		assert( #self.o       == self.len-1, "Removing element must decrease length" )
		assert( self.o[klt]   == vlt,        "Removing element musn't affect lower keys" )
		assert( self.o[idx-1] == vlt,        "Removing element musn't affect lower keys" )
		assert( self.o[idx]   == vgt,        "Removing element must move down higher keys" )
		assert( self.o[kgt]   == vgt,        "Removing element must keep keys attached" )
	end,

	DeleteIndexElement = function( self )
		Test.describe( "Removing Index element decreases size and move down higher keys" )
		local o     = self.o
		local idx   = math.ceil( self.len / 2 )
		local key   = self.keys[ idx   ]
		local val   = o[ idx   ]
		local klt   = self.keys[ idx-1 ]
		local kgt   = self.keys[ idx+1 ]
		local vlt   = o[ klt ]
		local vgt   = o[ kgt ]

		assert( o[key]   ~= nil,     "Element must exist before removing it" )
		o[ idx ]         =  nil
		assert( #o       == self.len-1, "Removing element must decrease length" )
		assert( o[klt]   == vlt,        "Removing element musn't affect lower keys" )
		assert( o[idx-1] == vlt,        "Removing element musn't affect lower keys" )
		assert( o[kgt]   == vgt,        "Removing element musn't affect lower keys" )
		assert( o[idx]   == vgt,        "Removing element must move down higher keys" )
		assert( o[key]   == nil,        "Element mustn't exist after removing it" )
	end,

	InsertElement = function( self )
		Test.describe( "Insert element increases size and move up higher keys" )
		local k2    = self.keys[2]
		local k3    = self.keys[3]
		local v2    = self.o[2]
		local v3    = self.o[3]
		local value = "oddth"
		local key   = 'odd'
		local idx   = 3

		Oht.insert( self.o, idx, key, value )
		assert( #self.o       == self.len+1, "Inserting element must increase length" )
		assert( self.o[key]   == value,      "Inserted  element must be available by key" )
		assert( self.o[idx]   == value,      "Inserted  element must be available by index" )
		assert( self.o[k2]    == v2,         "Inserting element musn't affect lower keys" )
		assert( self.o[idx-1] == v2,         "Inserting element musn't affect original lower keys" )
		assert( self.o[k3]    == v3,         "Inserting element musn't affect original higher keys" )
		assert( self.o[idx+1] == v3,         "Inserting element must move up higher keys" )
	end,

	InsertWithExistingKeyFails = function( self )
		Test.describe( "Insert element with existing key fails" )
		local idx   = math.random( 3, #self.o-3 )
		local key   = self.keys[ idx+2 ]
		local fnc   = function(o,k,i) Oht.insert( o, k, i, "foobar" )end

		assert( not pcall( fnc, self.o, key, idx ), "Not allowed to insert element with duplicated key" )
	end,

	ReplaceHashElement = function( self )
		Test.describe( "Replace Hash element remains size, indexes and keys" )
		local idx    = math.random( 3, #self.o-3 )
		local key    = self.keys[idx]
		local val    = "thirdish"
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ key ] = val
		assert( self.len == #self.o, ("Length is %d but should be %d"):format( #self.o, self.len ) )
		for i=1,self.len do
			assert( self.o[i]       == vals[i], "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i], "Hashed  Values match after Replace" )
			assert( Oht.index(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	ReplaceIndexElement = function( self )
		Test.describe( "Replace Index element remains size, indexes and keys" )
		local idx    = 3
		local key    = self.keys[idx]
		local val    = "thirdish"
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ idx ] = val
		assert( self.len == #self.o, "Replacing indexed element does not change length" )
		for i=1,self.len do
			assert( self.o[i]       == vals[i], "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i], "Hashed  Values match after Replace" )
			assert( Oht.index(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	CreateIndexOutOfBoundElementFails = function( self )
		Test.describe( "Can't create an element with an index higher than length" )
		local oob_idx  = #self.o + 1
		local val      = "doesn't matter"
		local func     = function() self.o[ oob_idx ] = val end

		assert( not pcall( func ), "Not allowed to create elements with indexes" )
	end,

	GetValues = function( self )
		Test.describe( "GetValues() returns properly ordered list of values" )
		--Test.todo( "Test.equal should report arrays as equal" )
		local vals  = Oht.values( self.o )
		for i=1,#vals do
			assert( vals[i] == self.vals[ i ],
				("%s should equal %s"):format( tostring( vals[i] ), tostring( self.vals[i] ) ) )
		end
		--assert( Test.equal( vals == self.vals ), "getValues() shall return self.vals" )
	end,

	GetKeys = function( self )
		Test.describe( "GetKeys() returns properly ordered list of keys" )
		--Test.todo( "Test.equal should report arrays as equal" )
		local keys = Oht.keys( self.o )
		for i=1,#keys do
			assert( keys[i] == self.keys[ i ],
				("%s should equal %s"):format( tostring( keys[i] ), tostring( self.keys[i] ) ) )
		end
		--assert( Test.equal( keys == self.keys ), "getKeys() shall return self.keys" )
	end,

	GetTable = function( self )
		Test.describe( "GetTable() returns a complete table of keyvalue pairs" )
		local t = Oht.table( self.o )
		local i = 0

		for k,v in pairs( t ) do
			assert( v == self.o[ k ], "Key/Value from simple table must matched OrderedHashTable" )
			i = i+1
		end
		assert( #self.o == i,
			 ("Number of elements in simple table (%d) must be equal to number of elements in OrderedHashTable (%d)"):format( i, #self ) )
	end,

	Equals = function( self )
		Test.describe( "__eq metamethod properly comparse for equality" )
		self.o.inner = Oht( self.o )
		local o      = Oht( self.o )
		assert( self.o == o, "Original and clone must be equal" )
	end,

	NotEquals = function( self )
		Test.describe( "__eq metamethod properly compares for inequality" )
		local idx    = math.random( 3, #self.o-3 )
		local o      = Oht( self.o )
		o[idx]       = 'Not sixth anymore'

		assert( self.o ~= o, "Original and clone mustn't be equal" )
	end,

	NotEqualsRecursively = function( self )
		Test.describe( "__eq metamethod properly compares for inequality recursively" )
		local idx    = math.random( 3, #self.o-3 )
		self.o.inner = Oht( self.o )
		local o      = Oht( self.o )
		local key    = self.keys[ idx ]
		o.inner[key] = 'Not sixth anymore'
		assert( self.o ~= o, "Original and clone mustn't be equal" )
	end
}
