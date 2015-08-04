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
  var url = "http://transport.opendata.ch/v1/locations?x=" + x + "&y=" + y;
  console.log(url);
  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      var stations = json.stations;
      
      sendMessage({
        "code": 20, // array start/size
        "scope" : 0,
        "count": stations.length
      });
      for( var i = 0; i < stations.length; i++) {
        var station = stations[i];
        
        // shorten station names
        station.name = station.name.replace(/^Basel, /, '');
        station.name = station.name.replace(/^Bern, /, '');
        station.name = station.name.replace(/^Zürich, /, '');
        station.name = station.name.replace(/Bahnhof/, 'Bhf');
        
        var dictionary = {
          "code" : 21,
          "scope" : 0,
          "item" : i,
          "id" : parseInt(station.id,10),
          "name" : station.name,
          "distance" : station.distance
        };
        console.log(JSON.stringify(dictionary));
        sendMessage(dictionary);
      }
    }
  );
}

function unescapeHTML(input) {
  input = input.replace(/&#196;/g, 'Ä');
  input = input.replace(/&#214;/g, 'Ö');
  input = input.replace(/&#220;/g, 'Ü');
  input = input.replace(/&#228;/g, 'ä');
  input = input.replace(/&#245;/g, 'ö');
  input = input.replace(/&#252;/g, 'ü');
  return input;
}

// get departures of station from inofficial ZVV API
function getDeparturesZVV(stationId) {
  var url = 'http://online.fahrplan.zvv.ch/bin/stboard.exe/dny?input=' + stationId + '&dirInput=&maxJourneys=10&boardType=dep&start=1&tpl=stbResult2json';
  console.log(url);
  xhrRequest(url, 'POST',
    function(responseText) {
      //console.log(responseText);
      var json = JSON.parse(responseText);
      var stationName = unescapeHTML(json.station.name);
      console.log(stationName);
      //var departures = json.connections.slice(0,9);
      var departures = json.connections;
      //TODO: save in local storage for later retreival
      
      sendMessage({
        "code": 20, // array start/size
        "scope" : 1,
        "count": departures.length
      });
      
      for( var i = 0; i < departures.length; i++) {
        var dep = departures[i];
        
        var name = dep.product.line;
        if(!name) {
          name = dep.product.name;
        }
        name = name.replace(/     /, '');
        name = name.substring(0,4);
        
        var icon = dep.product.icon;
        icon = icon.replace(/^icon_/, '');
        
        var direction = unescapeHTML(dep.product.direction);
        // shorten direction names
        direction = direction.replace(/^Basel, /, '');
        direction = direction.replace(/^Bern, /, '');
        direction = direction.replace(/^St.Gallen, /, '');
        direction = direction.replace(/^Zürich, /, '');
        direction = direction.replace(/^Bahnhof/, 'Bhf');
        
        var color_fg = '000000'; 
        var color_bg = 'ffffff'; 
        if(dep.product.color) {
          color_fg = dep.product.color.fg;
          color_bg = dep.product.color.bg;
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
        
        var dictionary = {
          "code" : 21,
          "scope" : 1,
          "item" : i,
          "id" : i,
          "name" : name,
          "icon" : icon,
          "direction" : direction,
          "color_fg" : parseInt(color_fg, 16), //convert in hex value
          "color_bg" : parseInt(color_bg, 16), //convert in hex value
          "delay" : delay,
          "countdown" : parseInt(countdown),
          "time" : dep_time
        };
// //         console.log(JSON.stringify(dep));
        console.log(JSON.stringify(dictionary));
        // Send to Pebble
        sendMessage(dictionary);
      }      
      
      sendMessage({
        "code": 22, // array end
        "scope" : 1,
        "count": departures.length
      });
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
      case 1: // departures of station
          getDeparturesZVV(e.payload.id);
          break;
      default:
          break;
      }
      break;
  default:
      break;
  }
});