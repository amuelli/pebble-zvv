var keys = require('message_keys');
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

var g_current_favorites = [];
var g_language = 'en';
try { g_language = localStorage.getItem('zvv_language') || 'en'; } catch(ex) {}
var g_show_disruptions = true;
try { var _d = localStorage.getItem('zvv_disruptions'); if (_d !== null) g_show_disruptions = _d !== '0'; } catch(ex) {}

// var DEBUG_LOCATION = { lat: 47.3783, lon: 8.5403 }; // Uncomment for emulator testing
var DEBUG_LOCATION;

var CODE = { GET: 10, UPDATE: 11, ARRAY_START: 20, ARRAY_ITEM: 21, ARRAY_END: 22 };
var SCOPE = { STA: 0, FAV: 1, DEPS: 2, NOTE: 3, LANG: 4 };

function buildUrl(base, params) {
  var query = Object.keys(params).map(function(k) {
    return k + '=' + encodeURIComponent(params[k]);
  }).join('&');
  return base + '?' + query;
}

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var g_msg_buffer = [];
var g_msg_transaction = null;

function sendMessage(data, success, failure) {
  function sendNext() {
    g_msg_transaction = null;
    var next = g_msg_buffer.shift();
    if(next) {
      sendMessage(next);
    }
  }
  if(g_msg_transaction) {
    g_msg_buffer.push(data);
  } else {
    g_msg_transaction = Pebble.sendAppMessage(data,
      function(e) {
        console.log("Message sent for transactionId=" + e.data.transactionId);
        if(g_msg_transaction !== null && g_msg_transaction >= 0 && g_msg_transaction != e.data.transactionId)
          console.log("### Confused! Message sent which is not a current message. "+
              "Current="+g_msg_transaction+", sent="+e.data.transactionId);
        if(success)
          success();
        sendNext();
      },
      function(e) {
        console.log("Failed to send message for transactionId=" + e.data.transactionId +
            ", error is "+("message" in e.error ? e.error.message : "(none)"));
        if(g_msg_transaction !== null && g_msg_transaction >= 0 && g_msg_transaction != e.data.transactionId)
          console.log("### Confused! Message not sent, but it is not a current message. "+
              "Current="+g_msg_transaction+", unsent="+e.data.transactionId);
        if(failure === true) {
          if(success)
            success();
        } else if(failure)
          failure();
        sendNext();
      }
    );
    if(g_msg_transaction === undefined) {
      g_msg_transaction = -1;
    }
    console.log("transactionId="+g_msg_transaction+" for msg "+JSON.stringify(data));
  }
}

function cleanStationName(name) {
  return name
    .replace(/^Basel, /, '')
    .replace(/^Bern, /, '')
    .replace(/^St\.Gallen, /, '')
    .replace(/^Zürich, /, '')
    .replace(/Bahnhof/, 'Bhf');
}

function getStations(x, y) {
  var url = "http://transport.opendata.ch/v1/locations?x=" + x + "&y=" + y;
  console.log(url);
  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      var stations = json.stations.filter(function(station) {
        return station.id != null;
      });
      var dict = {};

      dict[keys.code] = CODE.ARRAY_START;
      dict[keys.scope] = SCOPE.STA;
      dict[keys.count] = stations.length;
      sendMessage(dict);
      for( var i = 0; i < stations.length; i++) {
        var station = stations[i];
        station.name = cleanStationName(station.name);

        dict = {};
        dict[keys.code] = CODE.ARRAY_ITEM;
        dict[keys.scope] = SCOPE.STA;
        dict[keys.item] = i;
        dict[keys.id] = parseInt(station.id,10);
        dict[keys.name] = station.name;
        dict[keys.distance] = station.distance == null ? -1 : station.distance;
        console.log(JSON.stringify(dict));
        sendMessage(dict);
      }
    }
  );
}

function getDeparturesHafas(stationId) {
  var url = buildUrl('https://zvv.hafas.cloud/restproxy/departureBoard', {
    format: 'json',
    accessId: 'OFPubique',  // API access token
    type: 'DEP_STATION',    // departures from a station
    duration: 1439,         // time window in minutes (just under 24h)
    maxJourneys: 10,        // max results returned
    lang: g_language,
    id: stationId
  });
  console.log(url);
  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      var departures = json.Departure || [];

      // Collect notes first so they can be sent before departures
      var seenMsgIds = {};
      var notes = [];
      departures.forEach(function(dep) {
        if (dep.Messages && dep.Messages.Message) {
          dep.Messages.Message.forEach(function(msg) {
            if (!seenMsgIds[msg.id] && msg.head) {
              seenMsgIds[msg.id] = true;
              var body = (msg.text || '')
                .replace(/<[^>]*>/g, ' ')
                .replace(/\s+/g, ' ')
                .trim()
                .substring(0, 200);
              var timeStr = '';
              if (msg.sTime) timeStr += msg.sTime.substring(0, 5);
              if (msg.eTime) timeStr += '-' + msg.eTime.substring(0, 5);
              notes.push({ head: msg.head.substring(0, 128), body: body, time: timeStr });
            }
          });
        }
      });

      // Send notes before departures so they appear first on-watch
      if (g_show_disruptions && notes.length > 0) {
        sendMessage({ [keys.code]: CODE.ARRAY_START, [keys.scope]: SCOPE.NOTE,
                      [keys.count]: notes.length });
        notes.forEach(function(note, i) {
          sendMessage({ [keys.code]: CODE.ARRAY_ITEM, [keys.scope]: SCOPE.NOTE, [keys.item]: i,
                        [keys.note]: note.head, [keys.body]: note.body, [keys.time]: note.time });
        });
      }

      var mappedDepartures = departures.map(function(dep) {
        var name = '';
        if (dep.ProductAtStop && dep.ProductAtStop.icon && dep.ProductAtStop.icon.txt) {
          name = dep.ProductAtStop.icon.txt.substring(0, 4).trim();
        } else if (dep.ProductAtStop && dep.ProductAtStop.name) {
          name = dep.ProductAtStop.name.substring(0, 4).trim();
        }

        var direction = cleanStationName(dep.direction || '').trim();

        var dep_time = (dep.time || '').substring(0, 5);
        var delay = 0;

        if(dep.rtTime && dep.time && dep.rtTime !== dep.time) {
          var schParts = dep.time.split(':');
          var rtParts = dep.rtTime.split(':');
          var schMins = parseInt(schParts[0]) * 60 + parseInt(schParts[1]);
          var rtMins = parseInt(rtParts[0]) * 60 + parseInt(rtParts[1]);
          delay = rtMins - schMins;
          if(delay < -120) delay += 1440;
        }

        if(delay > 0) {
          dep_time = dep_time + ' +' + delay + "'";
        }

        var depTimeStr = dep.rtTime || dep.time || '00:00';
        var depParts = depTimeStr.split(':');
        var depTimeSecs = parseInt(depParts[0]) * 3600 + parseInt(depParts[1]) * 60;

        var colorFg = '000000';
        var colorBg = 'ffffff';

        if(dep.ProductAtStop && dep.ProductAtStop.icon) {
          var icon = dep.ProductAtStop.icon;
          if(icon.foregroundColor && icon.foregroundColor.hex) {
            colorFg = icon.foregroundColor.hex.substring(1);
          }
          if(icon.backgroundColor && icon.backgroundColor.hex) {
            colorBg = icon.backgroundColor.hex.substring(1);
          }
        }

        var icon = (dep.ProductAtStop && dep.ProductAtStop.catOutL)
          ? dep.ProductAtStop.catOutL.toLowerCase()
          : '';

        return {
          name: name,
          icon: icon,
          direction: direction,
          colorFg: colorFg,
          colorBg: colorBg,
          delay: delay,
          depTimeSecs: depTimeSecs,
          time: dep_time
        };
      }).sort(function(a, b) {
        return a.depTimeSecs - b.depTimeSecs;
      });

      sendMessage({ [keys.code]: CODE.ARRAY_START, [keys.scope]: SCOPE.DEPS,
                    [keys.count]: mappedDepartures.length });
      for(var i = 0; i < mappedDepartures.length; i++) {
        var dep = mappedDepartures[i];
        sendMessage({
          [keys.code]: CODE.ARRAY_ITEM,
          [keys.scope]: SCOPE.DEPS,
          [keys.item]: i,
          [keys.id]: i,
          [keys.name]: dep.name,
          [keys.icon]: dep.icon,
          [keys.direction]: dep.direction,
          [keys.colorFg]: parseInt(dep.colorFg, 16),
          [keys.colorBg]: parseInt(dep.colorBg, 16),
          [keys.delay]: dep.delay,
          [keys.depTime]: dep.depTimeSecs || 0,
          [keys.time]: dep.time
        });
      }
      sendMessage({ [keys.code]: CODE.ARRAY_END, [keys.scope]: SCOPE.DEPS,
                    [keys.count]: mappedDepartures.length });
    }
  );
}

var g_last_lat = null;
var g_last_lon = null;

function getLocation(highAccuracy, onSuccess, onError) {
  if (DEBUG_LOCATION) {
    var delay = highAccuracy ? 800 + Math.random() * 700 : 200 + Math.random() * 300;
    setTimeout(function() {
      onSuccess({ coords: { latitude: DEBUG_LOCATION.lat, longitude: DEBUG_LOCATION.lon } });
    }, delay);
    return;
  }
  navigator.geolocation.getCurrentPosition(onSuccess, onError, {
    enableHighAccuracy: highAccuracy,
    maximumAge: 10000,
    timeout: 15000
  });
}

function handleLocation(lat, lon) {
  var dist = (g_last_lat !== null)
    ? Math.abs(lat - g_last_lat) + Math.abs(lon - g_last_lon)
    : Infinity;
  if (dist > 0.001) { // ~100m threshold
    g_last_lat = lat;
    g_last_lon = lon;
    console.log('lat= ' + lat + ' lon= ' + lon);
    getStations(lat, lon);
  } else {
    console.log('location unchanged, skipping refresh');
  }
}

function sendLanguage() {
  sendMessage({ [keys.code]: CODE.UPDATE, [keys.scope]: SCOPE.LANG, [keys.name]: g_language });
}

Pebble.addEventListener("ready", function(e) {
  sendLanguage();
  // Phase 1: fast low-accuracy fix
  getLocation(false,
    function(pos) {
      handleLocation(pos.coords.latitude, pos.coords.longitude);

      // Phase 2: refine with high-accuracy fix
      getLocation(true,
        function(pos2) {
          handleLocation(pos2.coords.latitude, pos2.coords.longitude);
        },
        function(err) {
          console.log('high-accuracy error (' + err.code + '): ' + err.message);
        }
      );
    },
    function(err) {
      console.log('location error (' + err.code + '): ' + err.message);
    }
  );
});

Pebble.addEventListener("appmessage", function(e) {
  console.log("Received message: " + JSON.stringify(e.payload));
  switch(e.payload.code) {
  case CODE.GET:
      switch(e.payload.scope) {
      case SCOPE.STA:
          getStations();
          break;
      case SCOPE.DEPS:
          getDeparturesHafas(e.payload.id);
          break;
      default:
          break;
      }
      break;
  default:
      break;
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e.response || e.response === 'CANCELLED') return;
  var settings;
  try { settings = clay.getSettings(e.response, false); } catch(ex) { return; }
  var lang = settings.LANGUAGE && settings.LANGUAGE.value;
  if (lang) {
    g_language = lang;
    try { localStorage.setItem('zvv_language', lang); } catch(ex) {}
    sendLanguage();
  }
  var disruptions = settings.DISRUPTIONS;
  if (disruptions !== undefined) {
    g_show_disruptions = !!disruptions.value;
    try { localStorage.setItem('zvv_disruptions', g_show_disruptions ? '1' : '0'); } catch(ex) {}
  }
  var raw = settings.FAVORITES && settings.FAVORITES.value;
  if (!raw) return;
  var stations;
  try { stations = JSON.parse(raw); } catch(ex) { return; }
  if (!Array.isArray(stations)) return;
  console.log('Config window returned: ' + stations.length + ' stations');
  g_current_favorites = stations;
  try { localStorage.setItem('zvv_favorites', JSON.stringify(stations)); } catch(ex) {}
  var dict = {};
  dict[keys.code] = CODE.ARRAY_START;
  dict[keys.scope] = SCOPE.FAV;
  dict[keys.count] = stations.length;
  sendMessage(dict);
  for (var i = 0; i < stations.length; i++) {
    var station = stations[i];
    station.name = cleanStationName(station.name);
    dict = {};
    dict[keys.code] = CODE.ARRAY_ITEM;
    dict[keys.scope] = SCOPE.FAV;
    dict[keys.item] = i;
    dict[keys.id] = parseInt(station.id, 10);
    dict[keys.name] = station.name;
    console.log(JSON.stringify(dict));
    sendMessage(dict);
  }
});
