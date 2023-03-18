const net = require('net')

let server = net.createServer((socket) => {
	socket.on('data', (data) => {
		console.log("Received: ", + data); 
	}); 

	console.log("Accepted Connection"); 
	socket.write("Hello from the server!"); 
}).listen(5000, () => console.log("Listening on 5000"));
