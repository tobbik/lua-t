local shts = require't.net'.sck.sht

-- set up aliases for the Read/Write shutdown directions
if shts.SHUT_RD then
	shts.r,shts.rd,shts.read = shts.SHUT_RD,shts.SHUT_RD,shts.SHUT_RD
end

if shts.SHUT_WR then
	shts.w,shts.wr,shts.write = shts.SHUT_WR,shts.SHUT_WR,shts.SHUT_WR
end

if shts.SHUT_RDWR then
	shts.rw, shts.rdwr, shts.readwrite = shts.SHUT_RDWR,shts.SHUT_RDWR,shts.SHUT_RDWR
end

return shts
