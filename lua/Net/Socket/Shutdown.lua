local shts = require't.net'.sck.sht


-- set up aliases for the Read/Write shutdown directions
if shts.SHUT_RD then
	shts[ shts.SHUT_RD ]    = "SHUT_RD"
	shts.r                  = shts.SHUT_RD
	shts.rd                 = shts.SHUT_RD
	shts.read               = shts.SHUT_RD
end

if shts.SHUT_WR then
	shts[ shts.SHUT_WR ]    = "SHUT_WR"
	shts.w                  = shts.SHUT_WR
	shts.wr                 = shts.SHUT_WR
	shts.write              = shts.SHUT_WR
end

if shts.SHUT_RDWR then
	shts[ shts.SHUT_RDWR ]  = "SHUT_RDWR"
	shts.rw                 = shts.SHUT_RDWR
	shts.rdwr               = shts.SHUT_RDWR
	shts.redwrite           = shts.SHUT_RDWR
end

return shts
