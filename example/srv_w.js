var http = require('http');

var l1 = "This is the first part"
var l2 = "Be done with this Http Request"

var server = http.createServer(function(req, res) {
	res.writeHead( 200 , {'Content-Length': 5*l1.length + l2.length} );
	res.write( l1 );
	res.write( l1 );
	res.write( l1 );
	res.write( l1 );
	res.write( l1 );
	res.end( l2 );
});

server.listen(8002);
