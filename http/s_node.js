// This is a modified NodeJS sample app that allows HTTP performance tests
const http    = require( 'http' )
const url     = require( 'url' )
const process = require( 'process' )
const util    = require( 'util' )

const p       = ("This is a simple dummy load that is meant to generate some load").repeat( 10 )
//curl -i -X GET "http://localhost:8001/newUser?username=matt&password=password"
//curl -i "http://localhost:8001/auth?username=matt&password=password"
//ab -k -c 20 -n 250 "http://localhost:8001/auth?username=matt&password=password"

var getUsers = function(n) {
	var users    = {};
	var makeWord = function() {
		var wrd = [];
		for (var i=0; i<Math.floor(Math.random() * (12 - 6 + 1)) + 6; i++)
			wrd[i] = String.fromCharCode( Math.floor(Math.random() * (123 - 32 + 1)) + 32 );
		return wrd.join('')
	};
	for (var x=0; x<n; x++)
		users[ makeWord() ] = makeWord()
	return users;
}

var users = getUsers(5000);
var r_cnt = 0;

var rot47 = function( pw )
{
	var ret = [ ];
	for( var i=0; i<pw.length; i++)
	{
		var k = pw.charCodeAt( i );
		// ret.push( String.fromCharCode( '!' + (k + '!' - 47) % 94 ) )
		ret.push( String.fromCharCode( 33 + (k + 14)%94 ) )
	}
	return ret.join( '' );
}

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
			return res.end();
			return res.end( "Server error bad arguments or empty result" );
		}

		if (users[ username ] === rot47( password )) {
			res.setHeader( "Content-Type", "text/plain; charset=utf-8" )
			res.end( util.format( "%s This user was authorized at %d\n", (++r_cnt).toString().padStart(7," "), Date.now() ) );
		}
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

		console.log( username, password )
		if (!username || !password || users[ username ])
		{
			res.statusCode = 400;
			res.end( util.format( "Creating a new user failed -> %s\n", users[ username ] ? "User already exists" : "Insufficient arguments" ) )
		}
		else
		{
			users[ username ] = rot47( password ) ;
			console.log( "Created User -> %s:%s", username, users[ username ] );
			res.end( util.format( "Created new user `%s`\n", username ) );
		}
	}
	else if( uri.pathname == "/multi" )
	{
		const multiplier = uri.query.multiplier ? parseInt(uri.query.multiplier, 10) : undefined

		if (! multiplier)
		{
			res.statusCode = 400;
			return res.end( "Server error bad arguments" );
		}
		else
		{
			res.statusCode = 200;
			return res.end( payload.repeat(multiplier) );
		}
	}
	else
	{
		res.statusCode = 404;
		res.end( "There is nothing to do here\n" );
	}
} );

httpServer.on('connection', ( socket ) => {
	// display Socket connection errors. Not that specific. If the client hangs up, it's an error
	// but the server shouldn't care and keep trucking..
	//socket.removeAllListeners('error');
	// socket.addListener('error', (err) => { console.log("Socket Error:", err) });
	// console.log("New connection established:", socket.server._connections);
} );

var port = (process.argv[ 2 ]) ? parseInt( process.argv[ 2 ], 10 ) : 8000;
httpServer.listen( port, '0.0.0.0', 5000, function ( ) { } );

