
#include "TurboNRF24L01.h"

/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      1
#define LNA_HCURR   0        
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

#define ADDR_LEN	5
#define CRC_CONFIG ((1 << EN_CRC) | (0 << CRCO))

//////////////////////////////////////////////////////////////////////////
TurboNRF24L01::TurboNRF24L01()
{
	m_Channel = 1;
	m_Payload = 16;
	m_IsSendState = 0;
}

void TurboNRF24L01::Begin(uint8_t ce, uint8_t csn)
{
	m_CE.Begin(ce);
	m_CSN.Begin(csn);

	m_CE.PinMode(OUTPUT);
	m_CSN.PinMode(OUTPUT);

	m_SPI.Begin();

	m_CE.Low();
	m_CSN.High();
}

void TurboNRF24L01::SetRecvAddr(const uint8_t * addr)
{
	m_CE.Low();
	WriteRegister(RX_ADDR_P1, addr, ADDR_LEN);
	m_CE.High();
}

void TurboNRF24L01::SetRecvAddr(const char * addr)
{
	SetRecvAddr((const uint8_t*)addr);
}

void TurboNRF24L01::SetSendAddr(const uint8_t * addr)
{
	WriteRegister(RX_ADDR_P0, addr, ADDR_LEN);
	WriteRegister(TX_ADDR, addr, ADDR_LEN);
}

void TurboNRF24L01::SetSendAddr(const char * addr)
{
	SetSendAddr((const uint8_t*)addr);
}

void TurboNRF24L01::SetChannel(uint8_t channel)
{
	m_Channel = min(channel, 127);
	// Set RF channel
	WriteRegister(RF_CH, m_Channel);
}

void TurboNRF24L01::SetPayloadSize(uint8_t payload)
{
	m_Payload = payload;
	// Set length of incoming payload 
	WriteRegister(RX_PW_P0, m_Payload);
	WriteRegister(RX_PW_P1, m_Payload);
}

void TurboNRF24L01::Listen()
{
	// Start receiver 
	PowerUpRX();
	FlushRX();
}

void TurboNRF24L01::Send(uint8_t * val)
{
	// Wait until last paket is send
	while (m_IsSendState)
	{
		uint8_t status = GetStatus();

		if ((status & ((1 << TX_DS) | (1 << MAX_RT))))
		{
			m_IsSendState = 0;
			break;
		}
	}

	m_CE.Low();

	PowerUpTX();// Set to transmitter mode, Power up
	FlushTX();

	m_CSN.Low();// Pull down chip select
	m_SPI.Send(W_TX_PAYLOAD);// Write cmd to write payload
	m_SPI.Send(val, m_Payload);// Write payload
	m_CSN.High();// Pull up chip select

	m_CE.High();// Start transmission
}

void TurboNRF24L01::GetRecvData(uint8_t * val)
{
	m_CSN.Low();// Pull down chip select
	m_SPI.Send(R_RX_PAYLOAD);// Send cmd to read rx payload
	m_SPI.Receive(val, m_Payload);// Read payload
	m_CSN.High();// Pull up chip select
	WriteRegister(STATUS, (1 << RX_DR));// Reset status register
}

bool TurboNRF24L01::IsSending()
{
	if (m_IsSendState)
	{
		uint8_t status = GetStatus();

		// if sending successful (TX_DS) or max retries exceded (MAX_RT).
		if ((status & ((1 << TX_DS) | (1 << MAX_RT))))
		{
			PowerUpRX();
			return false;
		}

		return true;
	}
	return false;
}

bool TurboNRF24L01::IsRecvReady()
{
	uint8_t status = GetStatus();

	// We can short circuit on RX_DR, but if it's not set, we still need
	// to check the FIFO for any pending packets
	if (status & (1 << RX_DR))
		return true;
	return !RXFifoEmpty();
}

void TurboNRF24L01::WriteRegister(uint8_t reg, const uint8_t * val, uint8_t len)
{
	m_CSN.Low();
	m_SPI.Send(W_REGISTER | (REGISTER_MASK & reg));
	m_SPI.Send(val, len);
	m_CSN.High();
}

void TurboNRF24L01::WriteRegister(uint8_t reg, uint8_t val)
{
	m_CSN.Low();
	m_SPI.Send(W_REGISTER | (REGISTER_MASK & reg));
	m_SPI.Send(val);
	m_CSN.High();
}

void TurboNRF24L01::ReadRegister(uint8_t reg, uint8_t * val, uint8_t len)
{
	m_CSN.Low();
	m_SPI.Send(R_REGISTER | (REGISTER_MASK & reg));
	m_SPI.Receive(val, len);
	m_CSN.High();
}

uint8_t TurboNRF24L01::ReadRegister(uint8_t reg)
{
	m_CSN.Low();
	m_SPI.Send(R_REGISTER | (REGISTER_MASK & reg));
	uint8_t val = m_SPI.Receive();
	m_CSN.High();
	return val;
}

void TurboNRF24L01::PowerUpRX()
{
	m_IsSendState = 0;
	m_CE.Low();
	WriteRegister(CONFIG, CRC_CONFIG | ((1 << PWR_UP) | (1 << PRIM_RX)));
	m_CE.High();
	WriteRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
}

void TurboNRF24L01::FlushRX()
{
	m_CSN.Low();
	m_SPI.Send(FLUSH_RX);
	m_CSN.High();
}

void TurboNRF24L01::PowerUpTX()
{
	m_IsSendState = 1;
	WriteRegister(CONFIG, CRC_CONFIG | ((1 << PWR_UP) | (0 << PRIM_RX)));
}

void TurboNRF24L01::FlushTX()
{
	m_CSN.Low();
	m_SPI.Send(FLUSH_TX);
	m_CSN.High();
}

void TurboNRF24L01::PowerDown()
{
	m_CE.Low();
	WriteRegister(CONFIG, CRC_CONFIG);
}

uint8_t TurboNRF24L01::GetStatus()
{
	return ReadRegister(STATUS);
}

bool TurboNRF24L01::RXFifoEmpty()
{
	uint8_t fifoStatus = ReadRegister(FIFO_STATUS);
	return (fifoStatus & (1 << RX_EMPTY));
}