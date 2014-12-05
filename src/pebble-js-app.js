Pebble.addEventListener("ready",
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    var configurationSTR = localStorage.getItem("big_flip_clock_config") || '{"showBattery": 1, "invert": 0, "swapDateDoW" : 0, "enableBtNotif": 1}';
    var configuration = JSON.parse(configurationSTR);
    
    
    //Load the remote config page
    Pebble.openURL("http://codecorner.galanter.net/pebble/big_flip_clock_config.htm?showBattery=" + configuration.showBattery + '&invert=' + configuration.invert + '&swapDateDoW=' + configuration.swapDateDoW + "&enableBtNotif=" + configuration.enableBtNotif);
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    //Get JSON dictionary
    var configuration = JSON.parse(decodeURIComponent(e.response));
    var configurationSTR = JSON.stringify(configuration);
    console.log("Configuration window returned: " + configurationSTR);
    
    if (configurationSTR != '{}') {
      
      localStorage.setItem("big_flip_clock_config", configurationSTR);
 
      //Send to Pebble, persist there
      Pebble.sendAppMessage(
          {
            "KEY_SHOW_BATTERY": configuration.showBattery,
            "KEY_INVERT": configuration.invert,
            "KEY_SWAP_DATE_DOW": configuration.swapDateDoW,
            "KEY_ENABLE_BT_NOTIF": configuration.enableBtNotif,
          },
        function(e) {
          console.log("Sending settings data...");
        },
        function(e) {
          console.log("Settings feedback failed!");
        }
      );
    }
  }
);