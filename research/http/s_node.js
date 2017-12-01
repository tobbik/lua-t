// This is a modified NodeJS sample app that allows HTTP performance tests
const http    = require( 'http' )
const url     = require( 'url' )
const process = require( 'process' )

//curl -i -X GET "http://localhost:8001/newUser?username=matt&password=password"
//curl -i "http://localhost:8001/auth?username=matt&password=password"
//ab -k -c 20 -n 250 "http://localhost:8001/auth?username=matt&password=password"

var rot47lot = [ ];
var users    = { };
var r_cnt    = 0;

var rot47init = function( )
{
	var input  = '!"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~';
	var output = 'PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO';
	for( var i=0; i<input.length; i++)
		rot47lot[ input.charCodeAt( i ) ] = output.charAt( i );
}

var rot47 = function( pw )
{
	var ret = [ ];
	for( var i=0; i<pw.length; i++)
		ret.push( rot47lot[ pw.charCodeAt( i ) ] );
	return ret.join('');
}

rot47init( );

httpServer = http.createServer( (req, res) => {
	var uri = url.parse( req.url, true )
	if (uri.pathname == "/auth")
	{
		let username   = uri.query.username || '';
		const password = uri.query.password || '';

		username = username.replace(/[!@#$%^&*]/g, '');

		if (!username || !password || !users[username])
		{
			res.statusCode = 400;
			res.end( )
		}

		if (users[ username ] === rot47( password ))
			res.end( (r_cnt++) + " This user was authorized" );
		else {
			res.statusCode = 401;
			res.end( "Authorization failead\n" );
		}
	}
	else if (uri.pathname == "/newUser")
	{
		let   username = uri.query.username || '';
		const password = uri.query.password || '';

		username = username.replace(/[!@#$%^&*]/g, '');

		console.log( username, password, users[ username ] )
		if (!username || !password || users[ username ])
		{
			res.statusCode = 400;
			res.end("Creating a new user failed\n")
		}
		else
		{
			users[ username ] = rot47( password ) ;
			console.log( "Created User -> %s:%s", username, users[ username ] );
			res.end( "Created new user `" +username+ "`\n" );
		}
	}
	else
	{
		res.statusCode = 404;
		res.end( "There is nothing to do here\n" );
	}
} );

var port = (process.argv[ 2 ]) ? parseInt( process.argv[ 2 ], 10 ) : 8000;
httpServer.listen( port, '0.0.0.0', 5000, function ( ) { } );

