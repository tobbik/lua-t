const http = require('http');

const keepAliveAgent = new http.Agent({ keepAlive: true });

const options = {
  host: '127.0.0.1',
  port: 10000,
  path: 'auth?username=mickey&password=goofey',
  agent: keepAliveAgent
};


const fireRequest = () => {

  http.get(options, (resp) => {
    let data = ''

    // A chunk of data has been recieved.
    resp.on('data', (chunk) => {
      data += chunk;
    });

    // The whole response has been received. Print out the result.
    resp.on('end', () => {
      console.log(data);
      fireRequest();
    });

  }).on("error", (err) => {
    console.log("Error: " + err.message);
  });
}

fireRequest();
