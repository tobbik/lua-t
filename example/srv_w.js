var http = require('http'),
    fs   = require('fs');

function repeat(s, n)
{
	var a = [];
	while(a.length < n)
	{
		a.push(s);
	}
	return a.join('');
}

var l1="This is the first part before we finish. A string te repeated soo man times\n"
var l2="This is the finish line of the response.\n"

var rp = 300
var rc = 10
var s00 = repeat( l1, rp )

var h00= http.createServer( function(req, res) {
	res.writeHead( 200 , {'Content-Length': rc*s00.length + l2.length} );
	for ( var i=0; i<rc; i++)
	{
		res.write( s00 );
	}
	res.end( l2 );
});



var s01 = repeat( l1, rp*rc )
var h01= http.createServer( function(req, res) {
	// res.writeHead( 200 , {'Content-Length': s01.length + l2.length} );
	res.end( s01 + l2 );
});



var fileName = 'buf.lua';
var stats    = fs.statSync( fileName );
var fSz      = stats.size;
var h02 = http.createServer( function( req, res ) {
	fs.readFile( fileName, "binary", function( err, data ) {
		if(err) {
			res.writeHead(500, {"Content-Type": "text/plain"});
			res.write(err + "\n");
			res.end();
			return;
		}
		res.writeHead( 200, {"Content-Length": stats.size} );
		res.end( data );
	} );
});

h00.listen(9000);
h01.listen(9001);
h02.listen(9002);
