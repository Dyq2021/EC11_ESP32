#include <Arduino.h>
#include <OneButton.h>
#include <ESP32Encoder.h>
#include <BleKeyboard.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WebServer.h>

#define EC11_RIGHT 13
#define EC11_LEFT 12
#define EC11_BUTTON 14

// put your variable definitions here
OneButton upButton(EC11_BUTTON, true);
ESP32Encoder encoder;
int lastEncoderValue = 0;
int now_count = 0;
BleKeyboard bleKeyboard("旋转按钮", "DYQ", 100);

// WIFI AP
const char *AP_ssid = "Dyqiang_01";
const char *AP_password = "0000000001";
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// WebServer
WebServer server(80); // Object of WebServer(HTTP port, 80 is defult)
const char *username = "admin";
const char *Auth_password = "admin";
String HTML = "<!DOCTYPE html>\
<html>\
<body>\
<h1>My First Web Server with ESP32 - AP Mode 😊</h1>\
<h2>Dyqiang</h2>\
</body>\
</html>";

void handle_root() // 响应回调函数
{
	if (!server.authenticate(username, Auth_password))
	{
		return server.requestAuthentication();
	}
	server.send(200, "text/html", HTML);
}
// Wifi Config
const char *ssid = "Tian";
const char *password = "0000000001";
void Wifi_Connect(const char *ssid, const char *password)
{
	WiFi.begin(ssid, password);
	Serial.println();
	Serial.print("Connecting to ");
	Serial.print(ssid);
	Serial.println(" ...");
	int i = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.print(++i);
		Serial.print(' ');
	}
	Serial.println('\n');
	Serial.println("Connection established!");
	Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

// Mqtt
const char *mqtt_server = "192.168.1.254";
const int mqtt_port = 1883;
const char *mqtt_user = "mqtt";
const char *mqtt_password = "mqtt";
const char *mqtt_clientID = "ESP32_EC11";
const char *mqtt_topic = "/ESP32/BTN";

void Mqtt_callback(char *topic, byte *payload, unsigned int length);
WiFiClient espClient;
PubSubClient client(mqtt_server, mqtt_port, Mqtt_callback, espClient);

void Mqtt_Connect(const char *mqtt_server, const int mqtt_port, const char *mqtt_user, const char *mqtt_password, const char *mqtt_clientID);
void Mqtt_Loop()
{
	if (!client.connected())
	{
		Mqtt_Connect(mqtt_server, mqtt_port, mqtt_user, mqtt_password, mqtt_clientID);
	}
	client.loop();
}

void Mqtt_callback(char *topic, byte *payload, unsigned int length)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++)
	{
		Serial.print((char)payload[i]);
	}
	Serial.println("------------------------");
}

void Mqtt_Connect(const char *mqtt_server, const int mqtt_port, const char *mqtt_user, const char *mqtt_password, const char *mqtt_clientID)
{
	Serial.printf("Mqtt_Connect to %s:%d..........\n", mqtt_server, mqtt_port);
	while (!client.connected())
	{
		if (client.connect(mqtt_clientID, mqtt_user, mqtt_password))
		{
			Serial.println("Mqtt connected...");
			client.subscribe(mqtt_topic);
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			delay(5000);
		}
	}
}

void Mqtt_Publish(const char *topic, const char *payload)
{
	if (!client.connected())
	{
		Serial.println("Mqtt_Publish failed because client not connected");
		return;
	}
	client.publish(topic, payload);
}

void Mqtt_Publish(const char *topic, const int Intpayload)
{
	if (!client.connected())
	{
		Serial.println("Mqtt_Publish failed because client not connected");
		return;
	}
	char payload[10];
	sprintf(payload, "%d", Intpayload);
	client.publish(topic, payload);
}

void OnClick();

// put function declarations here:
void initEC11()
{
	pinMode(EC11_BUTTON, INPUT_PULLUP);

	ESP32Encoder::useInternalWeakPullResistors = UP;
	encoder.attachSingleEdge(EC11_RIGHT, EC11_LEFT);

	upButton.attachClick(OnClick);
	bleKeyboard.begin();
}

void OnClick()
{
	bleKeyboard.write(KEY_MEDIA_MUTE);
	Mqtt_Publish(mqtt_topic, "Click");
	// bleKeyboard.write(KEY_NUM_ENTER);
	Serial.println("Click");
}

void EC11_Loop()
{
	upButton.tick();
	if (lastEncoderValue != encoder.getCount())
	{
		now_count = encoder.getCount();
		if (lastEncoderValue > now_count)
		{
			Serial.print("Turn Right  Encoder Value: ");
			Serial.println(now_count);
			bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
		}
		else if (lastEncoderValue < now_count)
		{
			Serial.print("Turn Left Encoder Value: ");
			Serial.println(now_count);
			bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
		}
		Mqtt_Publish(mqtt_topic, now_count);
	}
	lastEncoderValue = now_count;
	delay(10);
}

void setup()
{
	// put your setup code here, to run once:
	Serial.begin(9600);
	// Create SoftAP
	WiFi.softAP(AP_ssid, AP_password);
	WiFi.softAPConfig(local_ip, gateway, subnet);

	Serial.print("Connect to My access point: ");
	Serial.println(AP_ssid);

	// Wifi connect
	Serial.println("--------------------------");
	Serial.printf("Connect to %s\n", ssid);
	Wifi_Connect(ssid, password);

	// Mqtt
	Mqtt_Connect(mqtt_server, mqtt_port, mqtt_user, mqtt_password, mqtt_clientID);

	// EC11
	initEC11();

	// WebServer
	server.on("/", handle_root);

	server.begin();
	Serial.println("HTTP server started");
}

void loop()
{
	// put your main code here, to run repeatedly:
	EC11_Loop();
	Mqtt_Loop();
	server.handleClient();
}
