Pack = require't.Pack'
ar   = Pack( 'b', 8 )
sq   = Pack( 'i1','i2','i3','i4',a[7] )
st   = Pack( {a='f'}, {b='d'}, {c='i'}, {struct=sq[5]}, {arr=sq[2] } )
