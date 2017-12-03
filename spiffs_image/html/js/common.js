var wsUri = "ws://"+window.location.host+"/dev";
var output;

var ws;
var retries = 0;
var val = 0.0;

var rcvd_tm = 0;
var rd_ch = 0; //-2 = dac, -3 = int adc, 0-3 = ext adc

var interval_timeout = 0;

function setMsg(cls, text)
{
	sbox = document.getElementById('status_box');
	sbox.className = "alert alert-" + cls;
	sbox.innerHTML = text;
	//console.log(text);
}

function init()
{
	output = document.getElementById("output");

	wsOpen();

	setInterval(
		function() { 
			interval_timeout++;

			if(interval_timeout > 30*8){
				rd_ch = 0;
				interval_timeout = 0;
				return;
			}else if(rd_ch > 15){
				return;
			}

			if(ws === undefined || ws.readyState == 3 || retries++ > 70)
				wsOpen();
			if(ws.readyState != 1) 
				return;

			if(rcvd_tm > 0){
				rcvd_tm--;
				return;
			}

			rd_ch = get_cb(rd_ch);
		},
		200
	);
}

function do_refresh()
{
	rd_ch = 0;
	interval_timeout = 0;
	return;
}

function wsOpen()
{
	if (ws === undefined || ws.readyState != 0) {
		if (retries > 0)
			setMsg("error", "WebSocket timeout, retrying..");
		else
			setMsg("info", "Opening WebSocket..");

		ws = new WebSocket(wsUri);
		ws.onopen = function(evt) { onOpen(evt) };
		ws.onclose = function(evt) { onClose(evt) };
		ws.onmessage = function(evt) { onMessage(evt) };
		ws.onerror = function(evt) { onError(evt) };

		retries = 0;  
	}
}

function onOpen(evt)
{
//    writeToScreen("CONNECTED");
	setMsg("info", '<span style="color: red;">CONNECTED</span> ');
}

function onClose(evt)
{
//    writeToScreen("DISCONNECTED");
	setMsg("info", '<span style="color: red;">DISCONNECTED</span> ');
}

function onMessage(evt)
{
//    writeToScreen('<span style="color: blue;">RECEIVED:' + evt.data+'</span>');
	setMsg("info", '<span style="color: blue;">RECEIVED:</span> ' + evt.data);

	var ar = evt.data.split("=");
	var el = document.getElementById(ar[0]);
	set_label_val_cb(el, ar[1]);

	el = document.getElementById(ar[0] + "g");
	set_ctl_state_cb(el, ar[1]);

	//console.log(ar[0] + "g = " + el.value);

//    websocket.close();

	retries = 0;

	rcvd_tm = 0;
}

function onError(evt)
{
	setMsg("error", '<span style="color: red;">ERROR:</span> ' + evt.data);
}

function doSend(message)
{
//    writeToScreen("SENT: " + message); 
	setMsg("info", '<span style="color: blue;">SENT:</span> ' + message);

	if (ws.readyState == 3 || retries++ > 70)
		wsOpen();
	else if (ws.readyState == 1)
		ws.send(message);
}

function writeToScreen(message)
{
	var pre = document.createElement("p");
	pre.style.wordWrap = "break-word";
	pre.innerHTML = message;
	output.appendChild(pre);
}

window.addEventListener("load", init, false);
