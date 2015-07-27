t=require't'

b=t.Buffer( 'ABCDEFGH' )

a = t.Packer.Array( t.Packer.IntB(2), 4)

a(b)
for i,v in pairs(a) do print(i,v) end
