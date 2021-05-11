-- vim: ts=2 sw=2 sts=2 et

Tst  = require't.Tst'
Test = require't.Test'
pp   = require't.Table'.pprint
Time = require't.Time'
Loop = require't.Loop'
G    = require'gamb'

f = Tst('First test', function()
  assert(1 == 1, 'One should be equal one')
end)

s = Tst('Second test', function()
  Tst.skip("Let's skip it ... it's Monday!")
  assert(1 == 2, 'One shouldn\'t be equal two')
end)

c=Test.Case('DaTest', 'standard', function()
  assert(1 == 2, 'One shouldn\'t be equal two')
end)

e = Tst('Second test', function( x, y, z )
  print(x,y,z)
  assert(1 == 2, 'One shouldn\'t be equal two')
end)

a,b = e( )

-- ###############################
l = Loop( )
g = G( 'delay', function( done )
  print(Tst.getSource(done))
  local tm2,tm1 = Time( 2000 ), Time( 1000 )
  local f2  = function( d )
    print( 'Entering F2' )
    print( 'Executing done()' )
    --l:stop()
    d( )
  end
  local f1  = function( )
    print( 'Entering F1' )
  end
  l:addTimer( tm2, f2, done )
  l:addTimer( tm1, f1 )
  print( "Finished G" )
  l:run( )
  print( "Ran G" )
end )

g2 = G( 'delay2', function( done )
  print(Tst.getSource(done))
  local tm3,tm4 = Time( 3000 ), Time( 4000 )
  local f4  = function(  )
    print( 'Entering F4' )
    print( 'Executing done()' )
    --l:stop()
    done( )
  end
  local f3  = function( )
    print( 'Entering F3' )
  end
  l:addTimer( tm3, f3 )
  l:addTimer( tm4, f4 )
  print( "Finished G" )
  l:run( )
  print( "Ran G" )
end, true )

