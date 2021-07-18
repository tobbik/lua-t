lud = require'udlib'
gu  = debug.getuservalue
su  = debug.setuservalue

--x   = lud.udx(5000000)
--lud.walk(x)
--lud.walkx(x)
--
AvsB = function(n)
  local n   = n or 30000000
  local x   = lud.ud( n )
  lud.walk(x)
  lud.walkv(x)
  x = lud.insert( x, 15 )
  x = lud.insert( x, 29876453 )
  lud.walk(x)
  lud.walkv(x)
end

AvsB()

y   = lud.ud(30)
lud.walk( y, true )
y  = lud.insert( y, 15 )
lud.walk( y, true )
y  = lud.insert( y, 40 )
lud.walk( y, true )
y  = lud.insert( y, -1 )
lud.walk( y, true )
lud.adjust( y, 50 )
lud.walk( y, true )
--lud.inserts( y, -1 )
--lud.inserts( y, 40 )

--lud.insert(y,15)
--lud.walk(y)
-- xu, t = gu(x)
-- n     = 1
-- while xu do
--   print(xu, t)
--   xu, t = gu(xu)
--   n = n+1
-- end

