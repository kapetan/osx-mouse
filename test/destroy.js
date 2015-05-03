var mouse = require('../')();

mouse.on('move', function(x, y) {
	console.log('move', x, y);
});

setTimeout(function() {
	mouse.destroy(function() {
		console.log('destroy');
	});
}, 1000);
