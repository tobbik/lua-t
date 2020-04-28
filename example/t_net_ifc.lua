Table=require't.Table'
Interface=require't.Net.Interface'

ifs=Interface.list()
Table.pprint(ifs)

ifc=Interface.default()
print(ifc)
Table.pprint(ifc)
