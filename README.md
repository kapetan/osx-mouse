# osx-mouse

Mouse tracking for OS X. Receive the screen position of various mouse events. The events are also emitted while another application is in the foreground.

	npm install osx-mouse

# Requirements
- Not compatible with node 12. Try an older version, like v11.11.0.
- Ensure the application that runs this (like iterm, or Mac terminal) has accessibility enabled (Settings > Privacy > Accessibility)

# Usage

The module returns an event emitter instance.

```javascript
var mouse = require('osx-mouse')();

mouse.on('move', function(x, y) {
	console.log(x, y);
});
```

The program will not terminate as long as a mouse listener is active. To allow the program to exit, either call `mouse.unref` (works as `unref`/`ref` on a TCP server) or `mouse.destroy()`.

The events emitted are: `move`, `left-down`, `left-up`, `left-drag`, `right-up`, `right-down` and `right-drag`. For each event the screen coordinates are passed to the handler function.
