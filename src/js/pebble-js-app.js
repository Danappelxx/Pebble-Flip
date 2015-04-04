var transferInProgress = false;
var media_id_list = [];
var curr_message = 0;
var access_token;

Pebble.addEventListener('showConfiguration', function (e){
	Pebble.openURL('https://www.dvappel.me/flip/redirect');
});

Pebble.addEventListener('webviewclosed', function (e) {
	var access_token = e.response;
	
	Pebble.sendAppMessage(
		{
			ACCESS_TOKEN: access_token
		},
		function(e) {
			console.log('Sent access token: ' + access_token);
		},
		function(e) {
			console.log('Failed sending access_token: ' + access_token);
		}
	);
});

Pebble.addEventListener("ready", function(e) {
	console.log("NetDownload JS Ready");
});

function send_media_id_list_length(){
	var media_id_list_length = media_id_list.length;
	
	Pebble.sendAppMessage(
		{
			IMAGE_DATA_DL_LENGTH: media_id_list_length
		},
		function(e) {
			console.log('Successfully delivered url list');
		},
		function(e) {
			console.log('Unable to deliver url list. Error: ' + e);
		}
	);
}

function send_next_message(){
	
	if( media_id_list[curr_message] != undefined) {
		console.log('sending next message: ' + media_id_list[curr_message]);
	
		var media_id = media_id_list[curr_message];
	
		Pebble.sendAppMessage(
			{
				IMAGE_DATA: media_id
			},
			function(e) {
				console.log('Successfully delivered url list');
				curr_message++;
			},
			function(e) {
				console.log('Unable to deliver url list. Error: ' + e);
			}
		);
	} else {
		console.log('Media id list at current message is undefined');
		Pebble.sendAppMessage(
			{
				IMAGE_DATA_DL_STOP: 'Send all available media ids'
			},
			function(e) {
				console.log('Successfully sent stop signal.');
			},
			function(e) {
				console.log('Unable to deliver stop signal');
			}
		);
	}
}



function send_media_feed_id_list(){
	
	var request = new XMLHttpRequest();
	var url = construct_request_url();
	
	request.onreadystatechange = function (){
		if(request.readyState == 4 && request.status == 200) {
			var response = JSON.parse(request.responseText);
			media_id_list = response;
			send_media_id_list_length();
		}
	};
	
	request.open("GET", url, true);
	request.send();
}

function construct_request_url(){

	var base_url = 'https://www.dvappel.me/flip/media_feed_id_list_with_prefix_and_postfix?token=';

	var url = base_url + access_token;
	console.log('request url: ' + url);
	return url;
}








Pebble.addEventListener("appmessage", function(e) {
  console.log("Got message: " + JSON.stringify(e));
	
	
	
	if('IMAGE_DATA_RECEIVED' in e.payload) {
		send_next_message();
	}
	
	if('ACCESS_TOKEN_RECEIVED' in e.payload) {
		access_token = e.payload['ACCESS_TOKEN_RECEIVED'];
		send_media_feed_id_list();
	}

	
	
	
	
	
	
  if ('NETDL_URL' in e.payload) {
    if (transferInProgress == false) {
      transferInProgress = true;
      downloadBinaryResource(e.payload['NETDL_URL'], function(bytes) {
        transferImageBytes(bytes, e.payload['NETDL_CHUNK_SIZE'],
          function() { console.log("Done!"); transferInProgress = false; },
          function(e) { console.log("Failed! " + e); transferInProgress = false; }
        );
      },
      function(e) {
        console.log("Download failed: " + e); transferInProgress = false;
      });
    }
    else {
      console.log("Ignoring request to download " + e.payload['NETDL_URL'] + " because another download is in progress.");
    }
  }
});

function downloadBinaryResource(imageURL, callback, errorCallback) {
    var req = new XMLHttpRequest();
    req.open("GET", imageURL,true);
    req.responseType = "arraybuffer";
    req.onload = function(e) {
        console.log("loaded");
        var buf = req.response;
        if(req.status == 200 && buf) {
            var byteArray = new Uint8Array(buf);
            var arr = [];
            for(var i=0; i<byteArray.byteLength; i++) {
                arr.push(byteArray[i]);
            }

            console.log("Downloaded file with " + byteArray.length + " bytes.");
            callback(arr);
        }
        else {
          errorCallback("Request status is " + req.status);
        }
    }
    req.onerror = function(e) {
      errorCallback(e);
    }
    req.send(null);
}

function transferImageBytes(bytes, chunkSize, successCb, failureCb) {
  var retries = 0;

  success = function() {
    console.log("Success cb=" + successCb);
    if (successCb != undefined) {
      successCb();
    }
  };
  failure = function(e) {
    console.log("Failure cb=" + failureCb);
    if (failureCb != undefined) {
      failureCb(e);
    }
  };

  // This function sends chunks of data.
  sendChunk = function(start) {
    var txbuf = bytes.slice(start, start + chunkSize);
    console.log("Sending " + txbuf.length + " bytes - starting at offset " + start);
    Pebble.sendAppMessage({ "NETDL_DATA": txbuf },
      function(e) {
        // If there is more data to send - send it.
        if (bytes.length > start + chunkSize) {
          sendChunk(start + chunkSize);
        }
        // Otherwise we are done sending. Send closing message.
        else {
          Pebble.sendAppMessage({"NETDL_END": "done" }, success, failure);
        }
      },
      // Failed to send message - Retry a few times.
      function (e) {
        if (retries++ < 3) {
          console.log("Got a nack for chunk #" + start + " - Retry...");
          sendChunk(start);
        }
        else {
          failure(e);
        }
      }
    );
  };

  // Let the pebble app know how much data we want to send.
  Pebble.sendAppMessage({"NETDL_BEGIN": bytes.length },
    function (e) {
      // success - start sending
      sendChunk(0);
    }, failure);

}