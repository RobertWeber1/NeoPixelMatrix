#include <ArduinoWebsockets.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>

#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>

#include <chrono>

using milliseconds = std::chrono::milliseconds;
using time_point = std::chrono::steady_clock::time_point;


String displayPage =
"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
R"###(
<!DOCTYPE HTML>
<html lang="en">
<head>
	<meta charset="utf-8">
	<meta name='viewport' content='width=device-width'>
	<title>NeoPixelMatrix</title>
	<style>
		body{
			margin: 0;
			background: #EFEFEF;
		}

		#Connecting {
			width: 100%;
			background-color: grey;
			text-align: center;
			color: white;
		}

		#ConnectingContent {
			display: flex;
			justify-content: center;
			align-items: center;
		}

		.Spinner {
			margin-left: 5px;
			border: 6px solid #f3f3f3; /* Light grey */
			border-top: 6px solid #3498db; /* Blue */
			border-radius: 50%;
			width: 15px;
			height: 15px;
			animation: spin 4s linear infinite;
		}

		#ColorPicker {
			width: 100%;
			height: 30px;
		}


		#Connected{
			margin:0;
		}

		#Main{
			display: flex;
			justify-content: center;
			align-items: center;
			flex-direction: column;
		}

		#HeadLine{
			margin: 0;
			border: 0;
			width: 100%;
			background-color: #4AA87E;
			text-align: center;
			color: white;
		}

		@keyframes spin {
			0% { transform: rotate(0deg); }
			100% { transform: rotate(360deg); }
		}
	</style>
	<script>
		var connection = null;

		function open_web_socket()
		{
			connection =
				new WebSocket('ws://'+location.hostname+':8080/', ['Display']);
			var heartbeat_interval = null;
			var missed_heartbeats = 0;

			function check_heartbeat()
			{
				clearInterval(heartbeat_interval);

				missed_heartbeats = 0;
				heartbeat_interval =
					setInterval(function()
						{
							missed_heartbeats++;
							if (missed_heartbeats >= 3)
							{
								clearInterval(heartbeat_interval);
								heartbeat_interval = null;
								connection.close();
								open_web_socket();
							}
						}, 10000);
			}

			connection.onopen =
				function ()
				{
					connection.send('my date is ' + new Date());
					check_heartbeat();
					connected_event();
				};
			connection.onerror =
				function (error)
				{
					console.log('WebSocket Error ', error);
				};
			connection.onmessage =
				function (msg)
				{
					if(msg.data === '---Heatbeat---')
					{
						missed_heartbeats = 0;
						console.log("received heartbeat");
						connection.send('---Heatbeat---');
					}
					else
					{
						console.log("received message: " + msg.data);
						process_message(msg.data);
					}
				};
			connection.onclose =
				function ()
				{
					console.log('WebSocket closed');
					setTimeout(open_web_socket, 3000);
					disconnected_event();
				};
		}
		open_web_socket();

		function process_message(raw_data)
		{
			var obj = JSON.parse(raw_data);

			switch(obj.type)
			{
			case "State": process_state(obj.value); break;
			case "Color": process_color(obj.value); break;
			case "Text": process_text(obj.value); break;
			case "Pixels": process_pixels(obj.value); break;
			}
		}

		function process_state(state)
		{
			document.getElementById('State').innerHTML = 'State: ' + state;
			if((state === "Initial") | (state == "Idle"))
			{
				document.getElementById('TextInput').value = "";
			}
		}

		function process_color(color)
		{
			var value =
				'#'+
				(color.r).toString(16).padStart(2,'0')+
				(color.g).toString(16).padStart(2,'0')+
				(color.b).toString(16).padStart(2,'0');
			document.getElementById('ColorPicker').value = value;
		}

		function process_text(text)
		{}

		function process_pixels(pixels)
		{}

		function connected_event()
		{
			document.getElementById('Connecting').style.display = 'none';
			document.getElementById('Connected').style.display = 'block';
		}
		function disconnected_event()
		{
			document.getElementById('Connecting').style.display = 'block';
			document.getElementById('Connected').style.display = 'none';
		}

		function hexToRgb(hex)
		{
			var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
			return result ? {
				r: parseInt(result[1], 16),
				g: parseInt(result[2], 16),
				b: parseInt(result[3], 16)
			} : null;
		}

		function isValid(obj)
		{
			for(var key in obj)
			{
				if(obj.hasOwnProperty(key))
					return true;
			}
			return false;
		}

		function color_changed()
		{
			if(isValid(connection))
			{
				var c = hexToRgb(document.getElementById('ColorPicker').value);
				var msg =
				{
					type: "Color",
					value: c
				};
				console.log(msg);
				connection.send(JSON.stringify(msg));
			}
		}

		function send_text()
		{
			if(isValid(connection))
			{
				var text = document.getElementById('TextInput').value;
				var msg =
				{
					type: "Text",
					value: text
				};
				console.log(msg);
				connection.send(JSON.stringify(msg));
			}

		}

		//----------------------------------------------------------------------
		function initial()
		{
			connected_event();
			process_message('{"type": "State", "value": "Initial"}');
		}
		function idle()
		{
			connected_event();
			process_message('{"type": "State", "value": "Idle"}');
		}
		function web_text()
		{
			connected_event();
			process_message('{"type": "State", "value": "WebText"}');
		}

		function set_test_color(color)
		{
			connected_event();
			process_message(
				'{"type": "Color", "value": {"r":'+
					color.r+', "g":'+
					color.g+', "b":'+
					color.b+'}}');
		}
	</script>
</head>
<body>
	<div id='Connecting'>
		<div id='ConnectingContent'>
			<h3>Connecting</h3>
			<div class='Spinner'> </div>
		</div>
	</div>

	<div id='Connected' style='display:none'>
		<div id='Main'>
			<div id='HeadLine'>
				<h3 >Neo Pixel Matrix</h3>
			</div>
			<div id='Content'>
				<p id='State'> State: Unknown</p>
				<p id='Color'>
					<input type="color" id="ColorPicker" oninput='color_changed()'>
				</p>
				<p id='Text'>
					Text
					<input type='text' id='TextInput'>
					<button onclick='send_text()' >send</button>
				</p>
				<p id='Pixels'> Pixels </p>
			</div>
		</div>
	</div>

	<div class='footer' style="position:fixed; left:0; bottom:0; width:100%; background-color:grey; text-align:center;">
		<button onclick='connected_event()'>connected</button>
		<button onclick='disconnected_event()'>disconnected</button>
		<button onclick='initial()'>Initial</button>
		<button onclick='idle()'>Idle</button>
		<button onclick='web_text()'>WebText</button>
		<button onclick='set_test_color({r:255, g:0, b:0})'>Red</button>
		<button onclick='set_test_color({r:0, g:255, b:0})'>Green</button>
		<button onclick='set_test_color({r:0, g:0, b:255})'>Blue</button>
	</div>
</body>
</html>
)###";


template<
	uint16_t Width = 20,
	uint16_t Height = 14,
	uint8_t Pin = 0,
	uint8_t WSClientCount = 2,
	uint16_t HttpPort = 80,
	uint16_t WsPort = 8080>
struct System
{
	using milliseconds = std::chrono::milliseconds;
	using time_point = std::chrono::steady_clock::time_point;

	System() = default;
	void init()
	{
		ws_server.listen(WsPort);
		web_server.begin();
		matrix.begin();
		matrix.setTextWrap(false);
		matrix.setBrightness(80);
		matrix.setTextColor(matrix.Color(80,255,0));
	}

	void process()
	{
		if(sec <= std::chrono::steady_clock::now())
		{
			++count;
			sec = std::chrono::steady_clock::now() + milliseconds(1000);
		}
		process_pixels();
		process_web_server();
		process_ws_server();
		process_ws_clients();
	}

private:
	void process_pixels()
	{
		if(frame_deadline <= std::chrono::steady_clock::now())
		{
			switch(state)
			{
			case State::Initial:
				if(x + 17 > (matrix.width() - pixelsInText))
				{
					matrix.fillScreen(0);
					matrix.setCursor(--x, 0);
					matrix.print(initial_text);
					matrix.show();
				}
				else
				{
					x = matrix.width();
					state = State::Idle;
				}
				frame_deadline = std::chrono::steady_clock::now() + animation_delay;
				break;
			case State::Idle:
			{
				x = 0;
				char buff[6];
				snprintf(buff, 6, "%02d:%02d", (count % 3600) / 60, count % 60);
				Serial.print("time: ");
				Serial.println(buff);

				matrix.fillScreen(0);
				matrix.setCursor(0, 0);
				matrix.print(buff);
				matrix.show();
				frame_deadline = std::chrono::steady_clock::now() + milliseconds(1000);
			}
				break;
			case State::WebText:
				break;
			}
		}
	}

	void process_web_server()
	{
		auto client = web_server.available();
		if(client)
		{
			Serial.println("deliver web page");
		 	client.println(displayPage);
		}
	}

	void process_ws_server()
	{
		if(not ws_server.poll())
		{
			return;
		}

		int slot = get_ws_slot();
		if(slot == -1)
		{
			Serial.println("not able to accapt ws client");
			return;
		}

		Serial.println("ws_server.acccept");
		ws_clients[slot] = ws_server.accept();

		Serial.println("WS-Client connected:");
	}

	void process_ws_clients()
	{
		for(auto & client : ws_clients)
		{
			client.process();
		}
	}

	int get_ws_slot()
	{
		for(int i= 0; i<WSClientCount; ++i)
		{
			if(not ws_clients[i].available())
			{
				return i;
			}
		}

		return -1;
	}

private:
	enum class State
	{
		Initial,
		Idle,
		WebText,
	} state = State::Initial;

	WiFiServer web_server = WiFiServer(HttpPort);
	websockets::WebsocketsServer ws_server;
	Adafruit_NeoMatrix matrix =
		Adafruit_NeoMatrix(
			Width,
			Height,
			Pin,
			NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
			NEO_RGB + NEO_KHZ800);

	struct WSClient : websockets::WebsocketsClient
	{
		void init()
		{
			using namespace websockets;
			onEvent(
				[&](WebsocketsClient& c, WebsocketsEvent type, WSInterfaceString)
				{
					switch(type)
					{
					case WebsocketsEvent::ConnectionOpened:
						Serial.println("connected");
						c.send("Hallo from Server");

						break;
					case WebsocketsEvent::ConnectionClosed:
						Serial.println("disconnected");
						heartbeat_deadline = time_point::max();
						heartbeat_response_deadline = time_point::max();
						break;
					case WebsocketsEvent::GotPing:
						Serial.println("ping");
						break;
					case WebsocketsEvent::GotPong:
						Serial.println("pong");
						break;
					}
				});

			onMessage(
				[&](WebsocketsClient& client, WebsocketsMessage const& msg)
				{
					Serial.print("received message: ");
					Serial.println(msg.c_str());
					if(msg.data() == "---Heatbeat---")
					{
						heartbeat_response_deadline = time_point::max();
						heartbeat_deadline =
							hb_request_interval +
							std::chrono::steady_clock::now();
					}
					// else
					// {
					// 	client.send("Echo: ");
					// 	client.send(msg.c_str());
					// }
				});

			heartbeat_deadline = std::chrono::steady_clock::now();
  			heartbeat_response_deadline = time_point::max();
		}

		WSClient& operator=(const websockets::WebsocketsClient& other)
		{
			static_cast<websockets::WebsocketsClient&>(*this) = other;
			init();
		}

  		void process()
  		{
  			if(available())
  			{
   				poll();
	  			if(heartbeat_deadline <= std::chrono::steady_clock::now())
	  			{
	  				Serial.println("xxxxxxxxxx send heartbeat");
	  				send("---Heatbeat---");
	  				heartbeat_response_deadline =
	  					std::chrono::steady_clock::now() + hb_response_interval;
	  				heartbeat_deadline = time_point::max();
	  			}
	  			else if(heartbeat_response_deadline <= std::chrono::steady_clock::now())
	  			{
	  				Serial.println("heartbeat timed out!");
	  				close();
	  				heartbeat_deadline = time_point::max();
	  				time_point heartbeat_deadline = time_point::max();
	  			}
	  		}
  		}
  	private:
  		time_point heartbeat_deadline = std::chrono::steady_clock::now();
  		time_point heartbeat_response_deadline = time_point::max();

  		const milliseconds hb_request_interval = milliseconds(5000);
  		const milliseconds hb_response_interval = milliseconds(5000);
	};

	std::array<WSClient, WSClientCount> ws_clients;

	const milliseconds animation_delay = milliseconds(200);
	time_point frame_deadline = std::chrono::steady_clock::now();
	time_point sec = std::chrono::steady_clock::now();
	int count = 0;

	const String initial_text = String("Test | 1234 | ");
	String dispText = String("");
	int pixelsInText = (sizeof(initial_text) * 7) + 8;
	int x = 20;
};

System<> sys;

void setup(){
	Serial.begin(115200);
	delay(10);
	Serial.println();

	Serial.print("Setting soft-AP ... ");
	Serial.println(WiFi.softAP("AndroidAP1", "androidap") ? "Ready" : "Failed!");

	Serial.print("AP IP address: ");
	Serial.println(WiFi.softAPIP());
	Serial.println("WebServer Started");

	sys.init();
}

void loop(){

	sys.process();
}
