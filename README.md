# osx-mouse

Mouse tracking for OS X. Receive the screen position of various mouse events. The events are also emitted while another application is in the foreground.

Versions of this library prior to version 2.0.0 also run with Node.js version 9 and below. Version 2.0.0 and above are context-aware.

	npm install osx-mouse

# Usage

The module returns an event emitter instance.

```javascript
var mouse = require('osx-mouse')()

mouse.on('move', function(x, y) {
	console.log(x, y)
})
```

The program will not terminate as long as a mouse listener is active. To allow the program to exit, either call `mouse.unref` (works as `unref`/`ref` on a TCP server) or `mouse.destroy()`.

The events emitted are: `move`, `left-down`, `left-up`, `left-drag`, `right-up`, `right-down`, `right-drag`, `middle-up`, `middle-down` and `middle-drag`. For each event the screen coordinates are passed to the handler function.

# Limitations

From *macOS Mojave* and forward this library requires input capturing permissions which need to be granted manually.

E.g. in *macOS Catalina* when running from *Terminal*:

1. Open `System Preferences > Security & Privacy > Input Monitoring`
2. Add *Terminal* to the list
