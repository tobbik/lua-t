xt=require'xt'

b=xt.Buffer( 'ABCDEFGH' )

a = xt.Packer.Array( xt.Packer.IntB(2), 4)
