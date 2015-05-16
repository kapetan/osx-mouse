var mouse = require('../');

var m1 = mouse();
var m2 = mouse();

m1.on('move', function(x, y) {
	console.log('move_1', x, y);
});

m2.on('move', function(x, y) {
	console.log('move_2', x, y);
});

setTimeout(function() {
	m1.unref();
	console.log("unref_1");

	setTimeout(function() {
		m2.unref();
		console.log("unref_2");
	}, 1000);
}, 1000);
