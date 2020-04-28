fib = require 'fib'

-- old way of loading the native "mandellib"
--local f,e1,e2 =
--	package.loadlib(
--		"./fib.so",
--		"luaopen_fib"
--	)
--print (f,e1,e2)
-- 
--fib = f( )


for i in coroutine.wrap(fib) do
    print(i)
end
