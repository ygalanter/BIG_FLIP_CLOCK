module.exports = [
  {
    "type": "heading",
    "defaultValue": "Big Flip Clock"
  },
  {
    "type": "text",
    "defaultValue": "Colors use the full palette on color watches and are limited to black &amp; white automatically on black-and-white watches."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "COLOR_BG",
        "label": "Background",
        "sunlight": false,
        "defaultValue": "0x000055"
      },
      {
        "type": "color",
        "messageKey": "COLOR_TIME_BG",
        "label": "Time Background",
        "sunlight": false,
        "defaultValue": "0xAAFFFF"
      },
      {
        "type": "color",
        "messageKey": "COLOR_TIME",
        "label": "Time",
        "sunlight": false,
        "defaultValue": "0x005500"
      },
      {
        "type": "color",
        "messageKey": "COLOR_DATE",
        "label": "Date",
        "sunlight": false,
        "defaultValue": "0xFFAA00"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Options"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_BATTERY",
        "label": "Show Battery Level",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "SWAP_DATE_DOW",
        "label": "Swap Date and Day of Week",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "ENABLE_BT_NOTIF",
        "label": "Bluetooth Status Alert",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
