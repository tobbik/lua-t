var http = require('http');

var server = http.createServer(function(req, res) {
	//var e = function() {
	//	res.end("The end");
	//}
	//res.writeHead(200);
	//res.write('Hello Http');
	//res.write('Some more Http');
	//res.write('Almost Done with Http');
	//res.write('Be Done with Http');
	//setTimeout( e, 5000 );
	//res.write('The first Part before we finish');
	res.write( 'This is my answer' );
	res.end( )
});

server.listen(8000);
