---
-- \file    t_set.lua
-- \brief   Test for the Set functionality
local T     = require( 't' )
local Rtvg  = T.require( "rtvg" )
local Table = require( 't.Table' )
local Test  = require( "t.Test" )
local Set   = require( "t.Set" )


local splitArray = function( arr )
	local part1, part2 = {},{}
	for i,v in pairs( arr ) do
		if math.random(1,1000) % 2 == 1 then
			table.insert( part1, v )
		else
			table.insert( part2, v )
		end
	end
	return part1, part2
end

	-- ----
return {
	len        = 5000,
	beforeAll  = function( self )
		self.rtvg = Rtvg( )
		-- self.rtvg.getVals() guarantees disjoint arrays!
		self.aryA = self.rtvg:getVals( self.len )
		self.aryB = self.rtvg:getVals( self.len )
		assert( self.len == #self.aryA, "Length of initial values must be the same" )
		assert( self.len == #self.aryB, "Length of initial values must be the same" )
	end,

	beforeEach = function( self )
		self.setA =  Set( self.aryA )
		self.setB =  Set( self.aryB )
	end,

-------------------------------------------------------------------
	-- Constructor Tests
	-- -----------------------------------------------------------------------

	ConstructEmptySet = function( self )
		Test.describe( "Construct the Empty Set" )
		local eSet = Set()
		assert( "t.Set" == T.type( eSet ), "Type t.Set expected" )
		assert( #eSet == 0, "Length must be zero" )
		for i,v in pairs( eSet ) do
			assert( false, "No iteration over Empty Set" )
		end
	end,

	ConstructEmptySetFromEmptyTable = function( self )
		Test.describe( "Construct Empty Set From Empty Array" )
		local eSet = Set( {} )
		assert( "t.Set" == T.type( eSet ), "Type t.Set expected" )
		assert( #eSet == 0, "Length must be zero" )
		for i,v in pairs( eSet ) do
			assert( false, "No iteration over Empty Set" )
		end
		assert( eSet == Set(), "Set from empty table must be equal Empty set" )
	end,

	ConstructorFromArray = function( self )
		Test.describe( "Construct Set from Array" )
		local set = Set( self.aryA )
		assert( "t.Set" == T.type( set ), "Type t.Set expected" )
		assert( #set == #self.aryA, "Length must be equal number of elements in array" )
		for i,v in ipairs( self.aryA ) do
			assert( set[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	ConstructorFromTable = function( self )
		Test.describe( "Construct Set from (Hash) Table" )
		local tbl = self.rtvg:getHash( self.len )
		local set = Set( tbl )
		assert( "t.Set" == T.type( set ), "Type t.Set expected" )
		assert( #set == self.len, "Length must be ".. self.len )
		for k,v in pairs( tbl ) do
			assert( set[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	ConstructorFromMixTable = function( self )
		Test.describe( "Construct Set from Mixed (Hash+Numeric) Table" )
		local tbl = self.rtvg:getHash( self.len )
		for k=1,self.len do table.insert( tbl, self.rtvg:getVal() ) end
		local set = Set( tbl )
		assert( "t.Set" == T.type( set ), "Type t.Set expected" )
		assert( #set == self.len*2, "Length must be "..self.len*2 )
		for k,v in pairs( tbl ) do
			assert( set[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	ConstructorByCloning = function( self )
		Test.describe( "Construct Set from existing set" )
		local lSet, cnt = Set( self.setB ), 0
		assert( "t.Set" == T.type( lSet ), "Type t.Set expected" )
		assert( #self.setB == #lSet, "Length must be equal number elements in original set" )
		for i,v in ipairs( self.aryB ) do
			assert( self.setB[ v ], "Element '"..tostring(v).."' must exist in set" )
			cnt = cnt+1
		end
		assert( cnt == #lSet, ("Iterated length %d must be equal number elements in original set %d"):format( cnt, #lSet ) )
	end,

	ConstructorRoundTrip = function( self )
		Test.describe( "Turn Set into table and construct an equal Set from table" )
		local tbl = Set.values( self.setA )
		assert( #self.setA == #tbl, "Table length must be equal number elements in original set" )
		for i,v in ipairs( tbl ) do
			assert( self.setA[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
		local cSet = Set( tbl )
		assert( "t.Set" == T.type( cSet ), "Type t.Set expected" )
		assert( cSet == self.setA, "New set must be equal to original set" )
	end,

	ConstructorRemovesDuplicates = function( self )
		Test.describe( "Constructing Set from table removes duplicate values" )
		local dupes = {}
		for i=0,(self.len*2)-1 do
			dupes[ i+1 ] = self.aryB[ i%self.len + 1 ]
		end
		assert( #dupes == 2*self.len, "New values must be double size of original array" )
		local lSet = Set( dupes )
		assert( "t.Set" == T.type( lSet ), "Type t.Set expected" )
		assert( #lSet == self.len, "New Set must be size of original array" )
		assert( lSet == self.setB, "New Set must be equal to setB" )
	end,

	proxyTableByIndexIsSet = function( self )
		Test.describe( "A sets proxyTable is indexed by t.Table.proxyTableIndex" )
		local set = Set( {'a','b','c'} )
		local prx = set[ Table.proxyTableIndex ]
		assert( Table.count( prx ) == 3, "proxyTable should have 3 members" )
		assert(true==prx.a and true==prx.b and true==prx.c, "all three, values should be `true`" )
	end,

	-- -----------------------------------------------------------------------
	-- Manipulate Elements of Set Add/Remove etc
	-- -----------------------------------------------------------------------

	AddElement = function( self )
		Test.describe( "Test adding elements to set" )
		local addA = self.rtvg:getVals( self.len )

		for i,v in pairs( addA ) do self.setA[ v ] = true end
		self.setA[ self.setB ] = true  -- nesting another T.Set

		assert( #self.setA == 2*self.len + 1, "Length of SetA shall be " .. 2*self.len + 1 ..
			" but is " .. #self.setA )
		for i,v in pairs( addA ) do
			assert( self.setA[ v ], "Element '".. tostring( v ) .."' shall exist" )
		end
	end,

	AddExistingElements = function( self )
		Test.describe( "Try adding existing set elements don't get added" )
		local lSet =  Set( self.setA )
		for i=1,self.len do
			lSet[ self.aryA[ i ] ] = true
		end
		assert( #self.setA == self.len, "Adding existing values shall not alter the Set" )
	end,

	RemoveElements = function( self )
		Test.describe( "Test removing elements from set" )

		local keepA,removeA = splitArray(self.aryA)
		local keepB,removeB = splitArray(self.aryB)
		for i,v in pairs( removeA ) do self.setA[ v ] = nil end
		for i,v in pairs( removeB ) do self.setB[ v ] = nil end

		assert( #self.setA == self.len - #removeA, ("Length of SetA (%d) shall be %d"):format( #self.setA, self.len - #removeA ))
		assert( #self.setB == self.len - #removeB, ("Length of SetB (%d) shall be %d"):format( #self.setB, self.len - #removeB ))

		for i,v in pairs( keepA ) do
			assert( self.setA[ v ], "Element '".. tostring( v ) .."' shall exist" )
		end
		for i,v in pairs( keepB ) do
			assert( self.setB[ v ], "Element '".. tostring( v ) .."' shall exist" )
		end
	end,

	RemoveNonExistingElements = function( self )
		Test.describe( "Removing non existing elements from set shall have no effect" )
		local aSet = Set( self.setA )
		local bSet = Set( self.setB )

		local keepA,removeA = splitArray(self.aryA)
		local keepB,removeB = splitArray(self.aryB)
		for i,v in pairs( removeB ) do self.setA[ v ] = nil end
		for i,v in pairs( removeA ) do self.setB[ v ] = nil end

		assert( self.setA == aSet, "SetA should still be identical ")
		assert( self.setB == bSet, "SetB should still be identical ")
	end,

	-- -----------------------------------------------------------------------
	-- Comparative operators
	-- -----------------------------------------------------------------------

	AreEqual = function( self )
		Test.describe( "Two sets are equal" )
		local aSet1 = Set( self.aryA )
		local aSet2 = Set( self.aryA )

		assert( aSet1 == aSet2, "Sets should be equal" )
		for i,v in ipairs( self.aryA ) do
			assert( aSet1[ v ], "Element '"..tostring(v).."' must exist in set" )
			assert( aSet2[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	AreNotEqual = function( self )
		Test.describe( "One element makes sets not equal" )
		local aSet = Set( self.setA )

		assert( aSet == self.setA, "Sets should be equal" )
		aSet[ self.rtvg:getVal() ] = true
		assert( aSet ~= self.setA, "Sets should not be equal" )
	end,

	AreDisjoint = function( self )
		Test.describe( "Two sets with entirely different elements are disjoint" )
		local aSet = self.setA
		local bSet = self.setB

		assert( self.setA % self.setB, "Set should be disjoint" )
	end,

	AreNotDisjoint = function( self )
		Test.describe( "One common elements makes sets not disjoint" )
		local aSet = self.setA
		local bSet = self.setB

		aSet[ self.aryB[1] ] = true   -- add one overlapping element

		assert( not (self.setA % self.setB), "Set should not be disjoint" )
	end,

	IsSubset = function( self )
		Test.describe( "Test set for being subset" )
		local aSet = Set( self.setA ) -- clone Set

		assert( #self.setA == #aSet, "Length of both sets must be equal" )

		assert( aSet == self.setA, "SetB should be equal setA" )
		assert( aSet <= self.setA, "SetB should be a Subset of setA" )
		assert( not (aSet < self.setA), "SetB is a Subset of setA, but not a true subset because it's equal" )
	end,

	IsNotSubset = function( self )
		Test.describe( "Test set for not being subset" )

		assert( not (self.setB <  self.setA), "SetB is not a Subset of setA" )
		assert( not (self.setB <= self.setA), "SetB is not a true Subset of setA" )
	end,

	IsTrueSubset = function( self )
		Test.describe( "Test set for being a true subset" )
		local aSet      = Set( self.setA ) -- clone Set
		local _,removeA = splitArray( self.aryA )

		assert( #self.setA == #aSet, "Length of both sets must be equal" )
		for i,v in pairs( removeA ) do self.setA[ v ] = nil end

		assert( self.setA ~= aSet, "self.setA should not be equal aSet" )
		assert( self.setA <= aSet, "self.setA should be a Subset of aSet" )
		assert( self.setA <  aSet, "self.setA should be a true Subset of aSet" )
	end,

	IntersectDisjointShallCreateEmptySet = function( self )
		Test.describe( "The Intersection of 2 disjoint sets shall have no Elements" )

		local iSet = self.setA & self.setB

		assert( 0 == #iSet, "Length of Intersection shall be 0" )
	end,

	-- -----------------------------------------------------------------------
	-- Operators with results
	-- -----------------------------------------------------------------------

	Intersection = function( self )
		Test.describe( "The Intersection of 2 sets shall have only common Elements" )
		local aSet = Set( self.setA )

		local xRemove,aKeep     = splitArray( self.aryA )
		local aRemove1,aRemove2 = splitArray( xRemove )
		for i,v in pairs( aRemove1 ) do      aSet[ v ] = nil end
		for i,v in pairs( aRemove2 ) do self.setA[ v ] = nil end

		local iSet = self.setA & aSet

		assert( self.len-#xRemove == #iSet, "Length of Intersection shall be ".. #self.setB-4 )
		for i,v in ipairs( xRemove ) do
			assert( not iSet[ v ], "Element '"..tostring(v).."' mustn't exist in Intersection" )
		end
		for i,v in ipairs( aKeep ) do
			assert( iSet[ v ], "Element '"..tostring(v).."' must exist in Intersection" )
		end
	end,

	IntersectionEqual = function( self )
		Test.describe( "The Intersection of 2 identical sets is the same set" )
		local aSet = Set( self.setA )
		local iSet = self.setA & aSet

		assert( self.setA == iSet, "Set and Intersection shall be equal" )
	end,

	IntersectionDisjunct = function( self )
		Test.describe( "The Intersection of 2 disjunct sets is the empty set" )
		local iSet = self.setA & self.setB

		assert( Set( ) == iSet, "Set and Intersection shall be equal" )
	end,

	Union = function( self )
		Test.describe( "The Union of 2 sets shall remove duplicate elements" )
		local aSet1 = Set( self.setA )
		local aSet2 = Set( self.setA )
		local add1  = self.rtvg:getVals( self.len )
		local add2  = self.rtvg:getVals( self.len )
		for i,v in pairs( add1 ) do aSet1[ v ] = true end
		for i,v in pairs( add2 ) do aSet2[ v ] = true end
		local uSet = aSet1 | aSet2

		assert( #self.setA + #add1 + #add2 == #uSet, "Length of Union shall be ".. #self.setA + #add1 + #add2..
			" but is " .. #uSet )
		for i,v in ipairs( self.aryA ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
		for i,v in ipairs( add1 ) do
			assert( uSet[ v ], "Add1 Element '"..tostring(v).."' must exist in set" )
		end
		for i,v in ipairs( add2 ) do
			assert( uSet[ v ], "Add2 Element '"..tostring(v).."' must exist in set" )
		end
	end,

	UnionEqual = function( self )
		Test.describe( "The Union of 2 equal sets shall be the same set" )
		local uSet = self.setA | self.setA

		assert( self.setA == uSet, "Union shall be equal both operands" )
	end,

	UnionDisjunct = function( self )
		Test.describe( "The Union of 2 disjunct sets shall have the sum of their elements" )
		local uSet = self.setA | self.setB

		assert( #self.setA*2 == #uSet, "Length of Union shall be ".. #self.setB*2 )
		for i,v in ipairs( self.aryA ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in union set" )
		end
		for i,v in ipairs( self.aryB ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in union set" )
		end
	end,

	Complement = function( self )
		Test.describe( "The Complement of 2 sets shall remove elements in SetB From SetA" )
		local aSet = Set( self.setA )

		local aRemove,aKeep     = splitArray( self.aryA )
		for i,v in pairs( aRemove ) do aSet[ v ] = nil end

		local cSet = self.setA - aSet

		assert( #aSet == #aKeep, "Length of aSet and aKeep shall be identical" )
		assert( #cSet == self.len - #aKeep, "Length of SetB shall be " .. self.len -#aKeep )
		for i,v in pairs( aRemove ) do
			assert( cSet[ v ], "Element '"..tostring(v).."' must exist in union set" )
		end
		for i,v in pairs( aKeep ) do
			assert( not cSet[ v ], "Element '"..tostring(v).."' must not exist in union set" )
		end
	end,

	ComplementEqual = function( self )
		Test.describe( "The Complement of 2 equal sets shall be empty" )
		local aSet = Set( self.setA )
		local cSet = self.setA - aSet

		assert( Set( ) == cSet, "Complement shall be Empty set" )
	end,

	ComplementDisjoint = function( self )
		Test.describe( "The Complement of 2 disjoint sets shall be equal 1st Set" )
		local cSet = self.setA - self.setB

		assert( self.setA == cSet, "Complement shall be equal SetB" )
	end,

	SymmetricDifference = function( self )
		Test.describe( "The Symmetric Difference of 2 equal sets shall be elements not in commmon" )
		local bSet1 = Set( self.setB )
		local bSet2 = Set( self.setB )
		local add1  = self.rtvg:getVals( self.len )
		local add2  = self.rtvg:getVals( self.len )
		for i,v in pairs( add1 ) do bSet1[ v ] = true end
		for i,v in pairs( add2 ) do bSet2[ v ] = true end

		local dSet = bSet1 ~ bSet2

		assert( #add1 + #add2 == #dSet, ("Length of difference shall be %d but was %d"):format( #add1 + #add2, #dSet ) )
		for i,v in ipairs( add1 ) do
			assert( dSet[ v ], ("Add1 Element '%s' must exist in set"):format( tostring(v) ) )
		end
		for i,v in ipairs( add2 ) do
			assert( dSet[ v ], "Add2 Element '"..tostring(v).."' must exist in set" )
		end
	end,

	SymmetricDifferenceEqual = function( self )
		Test.describe( "The Symmetric Difference of 2 equal sets shall be empty" )
		local aSet = Set( self.setA )
		local dSet = self.setA ~ aSet

		assert( Set(  ) == dSet, "Difference shall be Empty set" )
	end,

	SymmetricDifferenceDisjunct = function( self )
		Test.describe( "The Symmetric Difference of 2 disjunt sets shall be the union of the two" )
		local dSet = self.setA ~ self.setB
		local uSet = self.setA ~ self.setB
		--print( #dSet, Set.toString( dSet ) )
		--print( #uSet, Set.toString( uSet ) )

		assert( uSet == dSet, "Difference shall be union of sets" )
	end,
}

