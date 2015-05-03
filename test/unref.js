var mouse = require('../')();

mouse.on('move', function(x, y) {
	console.log('move', x, y);
});

setTimeout(function() {
	mouse.unref();
}, 1000);
