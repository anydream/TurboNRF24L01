
#include <TurboSPI.h>
#include <TurboNRF24L01.h>

TurboNRF24L01 g_rf24;

void setup()
{
	Serial.begin(9600);

	// pin CE connect to 8
	// pin CSN connect to 7
	g_rf24.Begin(8, 7);
	g_rf24.SetRecvAddr("clie1");
	g_rf24.SetChannel(1);
	g_rf24.SetPayloadSize(sizeof(unsigned long));
	g_rf24.Listen();

	Serial.println("Client Beginning ... ");
}

void loop()
{
	unsigned long time = millis();

	g_rf24.SetSendAddr("serv1");
	g_rf24.Send((byte*)&time);

	while (g_rf24.IsSending());
	Serial.println("Finished sending");
	delay(10);

	while (!g_rf24.IsRecvReady())
	{
		if ((millis() - time) > 1000)
		{
			Serial.println("Timeout!");
			return;
		}
	}

	g_rf24.GetRecvData((byte*)&time);

	Serial.print("Ping: ");
	Serial.println((millis() - time));

	delay(1000);
}
