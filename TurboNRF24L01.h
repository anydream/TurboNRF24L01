
#include <TurboSPI.h>

class TurboNRF24L01
{
public:
	TurboNRF24L01();

	// Init pins and chip. SPI use the ICSP pins, so no need to configure
	void Begin(uint8_t ce, uint8_t csn);
	// Setup receive address, 5 bytes length
	void SetRecvAddr(const uint8_t * addr);
	void SetRecvAddr(const char * addr);
	// Setup sending address, 5 bytes length
	void SetSendAddr(const uint8_t * addr);
	void SetSendAddr(const char * addr);

	// Setup channel
	void SetChannel(uint8_t channel);
	// Setup payload length
	void SetPayloadSize(uint8_t payload);

	// Start listen and enter receive mode
	void Listen();

	// Send data
	void Send(uint8_t * val);

	// Get received data
	void GetRecvData(uint8_t * val);

	// Query is sending
	bool IsSending();

	// Query is received data ready
	bool IsRecvReady();

private:
	void WriteRegister(uint8_t reg, const uint8_t * val, uint8_t len);
	void WriteRegister(uint8_t reg, uint8_t val);
	void ReadRegister(uint8_t reg, uint8_t * val, uint8_t len);
	uint8_t ReadRegister(uint8_t reg);

	void PowerUpRX();
	void FlushRX();
	void PowerUpTX();
	void FlushTX();
	void PowerDown();

	uint8_t GetStatus();
	bool RXFifoEmpty();

private:
	TurboSPI	m_SPI;
	DigitalPin	m_CE, m_CSN;
	uint8_t		m_Channel;
	uint8_t		m_Payload;
	uint8_t		m_IsSendState;
};
