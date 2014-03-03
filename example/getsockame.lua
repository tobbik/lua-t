#!out/bin/lua
xt=require'xt'
s=xt.Socket('TCP')
s:listen(5)
ip=s:getsockname()
