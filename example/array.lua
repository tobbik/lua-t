xt=require'xt'

b=xt.Buffer( 'ABCDEFGH' )

a = xt.Packer.Array( xt.Packer.IntB(2), 4)

a(b)
for i,v in pairs(a) do print(i,v) end
