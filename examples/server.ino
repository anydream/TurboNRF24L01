
#include <TurboSPI.h>
#include <TurboNRF24L01.h>

TurboNRF24L01 g_rf24;

void setup()
{
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);

	Serial.begin(9600);

	g_rf24.Begin(8, 7);
	g_rf24.SetRecvAddr("serv1");
	g_rf24.SetChannel(1);
	g_rf24.SetPayloadSize(sizeof(unsigned long));
	g_rf24.Listen();

	Serial.println("Server Listening...");
}

bool g_bLED = false;

void loop()
{
	unsigned long data;

	if (!g_rf24.IsSending() && g_rf24.IsRecvReady())
	{
		g_bLED = !g_bLED;
		digitalWrite(13, g_bLED);

		Serial.println("Got packet");
		g_rf24.GetRecvData((uint8_t*)&data);

		g_rf24.SetSendAddr("clie1");
		g_rf24.Send((uint8_t*)&data);

		Serial.println("Reply sent.");
	}
}
