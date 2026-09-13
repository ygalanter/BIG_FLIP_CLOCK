var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Clay auto-handles showConfiguration and webviewclosed, converting the color
// pickers to integers and forwarding all settings to the watch via AppMessage.
Pebble.addEventListener('ready', function() {
  console.log('Big Flip Clock: PebbleKit JS ready');
});
