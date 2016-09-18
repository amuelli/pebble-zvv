var keys = require('message_keys');
function xhrRequest (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
}

function encode(o, sep) {
  var list = [];
  var key;
  for (key in o) {
    if (o[key] != null && typeof o[key] != 'object' &&
      typeof o[key] != 'function') {
      list.push(encodeURIComponent(key) + '=' + encodeURIComponent(o[key]));
    }
  }
  return list.join(sep || '&');
}

var g_msg_buffer = [];
var g_msg_transaction = null;

/**
 * Sends appMessage to pebble; logs errors.
 * failure: may be True to use the same callback as for success.
 */
function sendMessage(data, success, failure) {
  function sendNext() {
    g_msg_transaction = null;
    var next = g_msg_buffer.shift();
    if(next) { // have another msg to send
      sendMessage(next);
    }
  }
  if(g_msg_transaction) { // busy
    g_msg_buffer.push(data);
  } else { // free
    g_msg_transaction = Pebble.sendAppMessage(data,
      function(e) {
        console.log("Message sent for transactionId=" + e.data.transactionId);
        //clearTimeout(msgTimeout);
        if(g_msg_transaction >= 0 && g_msg_transaction != e.data.transactionId) // -1 if unsupported
          console.log("### Confused! Message sent which is not a current message. "+
              "Current="+g_msg_transaction+", sent="+e.data.transactionId);
        if(success)
          success();
        sendNext();
      },
      function(e) {
        console.log("Failed to send message for transactionId=" + e.data.transactionId +
            ", error is "+("message" in e.error ? e.error.message : "(none)"));
        //clearTimeout(msgTimeout);
        if(g_msg_transaction >= 0 && g_msg_transaction != e.data.transactionId)
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
    if(g_msg_transaction === undefined) { // iOS buggy sendAppMessage
      g_msg_transaction = -1; // just a dummy "non-false" value for sendNext and friends
    }
//     var msgTimeout = setTimeout(function() {
//       console.log("Message timeout! Sending next.");
//       // FIXME: it could be really delivered. Maybe add special handler?
//       if(failure === true) {
//         if(success)
//           success();
//         } else if(failure) {
//           failure();
//         }
//         sendNext();
//     }, g_msg_timeout);
    console.log("transactionId="+g_msg_transaction+" for msg "+JSON.stringify(data));
  }
}

function getStations(x, y) {
  var url = "http://transport.opendata.ch/v1/locations?";
  console.log(url);
  var params = encode({ x: x, y: y });
  xhrRequest(url+params, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      var stations = json.stations;
      var dict = {};

      //TODO: better handling of unsuccessful requests
      dict[keys.code] = 20; // array start/size
      dict[keys.scope] = 0;
      dict[keys.count] = stations.length;
      sendMessage(dict);
      for( var i = 0; i < stations.length; i++) {
        var station = stations[i];

        // shorten station names
        station.name = station.name.replace(/^Basel, /, '');
        station.name = station.name.replace(/^Bern, /, '');
        station.name = station.name.replace(/^Zürich, /, '');
        station.name = station.name.replace(/Bahnhof/, 'Bhf');

        dict = {};
        dict[keys.code] = 21;
        dict[keys.scope] = 0;
        dict[keys.item] = i;
        dict[keys.id] = parseInt(station.id,10);
        dict[keys.name] = station.name;
        dict[keys.distance] = station.distance;
        console.log(JSON.stringify(dict));
        sendMessage(dict);
      }
    }
  );
}



function decodeHTMLSpecialCharacters(input) {
  //mapping of encoding of special character relevant to German, French and Italian
  var htmlEncoding = {
    '192' : 'À',
    '193' : 'Á',
    '194' : 'Â',
    '196' : 'Ä',
    '199' : 'Ç',
    '200' : 'È',
    '201' : 'É',
    '202' : 'Ê',
    '204' : 'Ì',
    '205' : 'Í',
    '210' : 'Ò',
    '211' : 'Ó',
    '212' : 'Ô',
    '214' : 'Ö',
    '220' : 'Ü',
    '223' : 'ß',
    '224' : 'à',
    '225' : 'á',
    '226' : 'â',
    '228' : 'ä',
    '231' : 'ç',
    '232' : 'è',
    '233' : 'é',
    '234' : 'ê',
    '236' : 'ì',
    '237' : 'í',
    '242' : 'ò',
    '243' : 'ó',
    '244' : 'ô',
    '246' : 'ö',
    '249' : 'ù',
    '250' : 'ú',
    '251' : 'û',
    '252' : 'ü'
  };
  return input.replace(/&#(\d{2,3});/g, function(match, p1){
    return htmlEncoding[p1];
  });
}

/**
 * get departures of station from inofficial ZVV API
 * @param {number} stationId station id to get departures from
 * @param {number=} dirStationId station id to get departures to
 * @param {number} max maximal numbers of departures
 */
function getDeparturesZVV(stationId, dirStationId, max) {
  var dirInput = dirStationId | '';
  var maxJourneys = max | '10';
  var url = 'http://online.fahrplan.zvv.ch/bin/stboard.exe/dny?';
  console.log(url);
  var params = encode({
    input: stationId,
    dirInput: dirStationId,
    maxJourneys: max,
    boardType: 'dep',
    start: 1,
    tpl: 'stbResult2json'
  });
  xhrRequest(url+params, 'POST',
    function(responseText) {
      //console.log(responseText);
      var json = JSON.parse(responseText);
      var stationName = decodeHTMLSpecialCharacters(json.station.name);
      console.log(stationName);
      //var departures = json.connections.slice(0,9);
      var departures = json.connections;
      //TODO: save in local storage for later retreival


      var dict = {};
      dict[keys.code] = 20; // array start/size
      dict[keys.scope] = 2;
      dict[keys.count] = departures.length;
      sendMessage(dict);

      for( var i = 0; i < departures.length; i++) {
        var dep = departures[i];

        var name = dep.product.line;
        if(!name) {
          name = dep.product.name;
        }
        name = name.replace(/     /, '');
        name = name.substring(0,4);
        //name = name.replace(/S /, 'S');

        var icon = dep.product.icon;
        icon = icon.replace(/^icon_/, '');

        var direction = decodeHTMLSpecialCharacters(dep.product.direction);
        // shorten direction names
        direction = direction.replace(/^Basel, /, '');
        direction = direction.replace(/^Bern, /, '');
        direction = direction.replace(/^St.Gallen, /, '');
        direction = direction.replace(/^Zürich, /, '');
        direction = direction.replace(/^Bahnhof/, 'Bhf');

        var colorFg = '000000';
        var colorBg = 'ffffff';
        if(dep.product.color) {
          colorFg = dep.product.color.fg;
          colorBg = dep.product.color.bg;
        }

        var realTime = dep.mainLocation.realTime;
        var countdown = dep.mainLocation.countdown;
        // set delay and adapt countdown if real time info available
        var delay = 0;
        if(realTime.hasRealTime) {
          delay = parseInt(realTime.delay);
          countdown = realTime.countdown;
        }

        // set departure time (with delay)
        var dep_time = dep.mainLocation.time;
        if(delay > 0) {
          dep_time = dep_time + ' +' + delay + "'";
        }

        dict = {};
        dict[keys.code] = 21;
        dict[keys.scope] = 2;
        dict[keys.item] = i;
        dict[keys.id] = i;
        dict[keys.name] = name;
        dict[keys.icon] = icon;
        dict[keys.direction] = direction;
        dict[keys.colorFg] = parseInt(colorFg, 16); //convert in hex value;
        dict[keys.colorBg] = parseInt(colorBg, 16); //convert in hex value;
        dict[keys.delay] = delay;
        dict[keys.countdown] = parseInt(countdown);
        dict[keys.time] = dep_time;

// //         console.log(JSON.stringify(dep));
        console.log(JSON.stringify(dict));
        // Send to Pebble
        sendMessage(dict);
      }

      dict = {};
      dict[keys.code] = 22;
      dict[keys.scope] = 2;
      dict[keys.count] = departures.length;
      sendMessage(dict);
    }
  );
}


var locationOptions = {
  enableHighAccuracy: true,
  maximumAge: 10000,
  timeout: 10000
};

function locationSuccess(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  getStations(pos.coords.latitude, pos.coords.longitude);
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

// Called when JS is ready
Pebble.addEventListener("ready",
    function(e) {
    // Request current position
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
});

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  console.log("Received message: " + JSON.stringify(e.payload));
  switch(e.payload.code) {
  case 10: // get info
      switch(e.payload.scope) {
      case 0: // nearest stations
          getStations();
          break;
      case 2: // departures of station
          getDeparturesZVV(e.payload.id, null, 10);
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
  // Show config page
  Pebble.openURL('http://188.166.34.8:5000');
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config_data = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + JSON.stringify(config_data));
  console.log('Config window returned: ' + JSON.stringify(config_data.stations));
  var stations = config_data.stations;
  console.log(stations.length);
  if(stations.length > 0) {
    var dict = {};
    dict[keys.code] = 20;
    dict[keys.scope] = 1;
    dict[keys.count] = stations.length;
    sendMessage(dict);
    for( var i = 0; i < stations.length; i++) {
      var station = stations[i];

      // shorten station names
      station.name = station.name.replace(/^Basel, /, '');
      station.name = station.name.replace(/^Bern, /, '');
      station.name = station.name.replace(/^Zürich, /, '');
      station.name = station.name.replace(/Bahnhof/, 'Bhf');

      dict = {};
      dict[keys.code] = 21;
      dict[keys.scope] = 1;
      dict[keys.item] = i;
      dict[keys.id] = parseInt(station.id,10);
      dict[keys.name] = station.name;
      console.log(JSON.stringify(dict));
      sendMessage(dict);
    }
  }
});
