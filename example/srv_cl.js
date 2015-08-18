var http   = require('http');
var answer = "This is my answer"

var server = http.createServer(function(req, res) {
	//var e = function() {
	//	res.end("The end");
	//}
	res.writeHead( 200,{"Content-Length": answer.length} );
	res.flushHeaders( );
	//res.write('Hello Http');
	//res.write('Some more Http');
	//res.write('Almost Done with Http');
	//res.write('Be Done with Http');
	//setTimeout( e, 5000 );
	res.write( answer );
	res.end( );
});

server.listen(8000);
