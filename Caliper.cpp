#include "Caliper.h"

// This class interfaces to cheap digital calipers with serial
// data output via a clock and data pin.
// James Ward 03/02/19
  
Caliper::Caliper(byte clockPin, byte dataPin) :
  m_clockPin(clockPin, INPUT),
  m_dataPin(dataPin, INPUT),
  m_packet(0)
{   
}

bool Caliper::update()
{
  // This function reads a data packet from the Caliper, it
  // expects 28 bits of data (comprising 7 sets of 4 bits,
  // the first 5 of which are the digits in BCD format).
  // This function also assumes the clock and data signals
  // are inverted.

  uint32_t start = 0;
  uint32_t ticks = 0;

  // block until the clock line has been high for longer
  // than 800us, which should put us at the start of a
  // new packet (the packets are 784us duration, and
  // sent every 320ms)
  do {
    m_clockPin.waitHigh();
    start = micros();
    m_clockPin.waitLow();
    ticks = micros();
  } while (ticks - start < 800);

  for (int retry=0; retry<4; retry++) {  
    // Read in 29 bits on the rising edge of the clock,
    // reversing the order of bits as we go (i.e. the
    // first bit sent by the calipers will end up in
    // the LSB). Only the first 28 bits are real data,
    // because the last bit is the final rising edge
    // of the clock at the end of the packet.
    const uint32_t msb = 1UL<<29;
    uint32_t value = 0;
    for (int i=0; i<29; i++) {
      m_clockPin.waitRisingEdge();
      if (m_dataPin.isLow()) value |= msb;
      value >>= 1;
    }
  
    // Calculate the total length of the packet, which
    // should be around 782us (or around 784us, given
    // the 4us granularity of the Arduino timer)
    ticks = micros() - ticks;
  
    // If the elapsed time is around 784us, it looks
    // like we received a complete packet. However, if
    // the elapsed time is a lot larger than 784us,
    // we must have started part way through a packet,
    // so the data will be invalid, and we need to recover.
    if (ticks < 900) {
      // looks good, remove the redundant bit and return
      m_packet = value & (msb-1);
      return true;
    } else {
      // the delay was too long, we must have started
      // part way through a packet, and will have ended
      // up part way through the next packet.
      // delay 10ms here to ensure we are clear of
      // the current packet, then retry
      delay(10);
      
      // wait for the start of the next packet, and
      // store the timestamp
      m_clockPin.waitLow();
      ticks = micros();
    }
  }

  // gave up after a number of retries
  m_packet = 0;
  return false;
}//update

uint32_t Caliper::getPacket() const
{
  return m_packet;
}

uint32_t Caliper::getDigitsBCD() const
{
  // the lower 20 bits contain 5 digits in BCD
  // format with 4 bits per digit
  return m_packet & 0xFFFFF;
}

bool Caliper::isNegative() const
{
  // bit 24 is the negative flag
  return (m_packet & 1UL<<24) != 0;
}

bool Caliper::isMetric() const
{
  // bit 26 is the metric (mm) flag
  return (m_packet & 1UL<<26) != 0;
}

bool Caliper::isInch() const
{
  return !isMetric();
}

bool Caliper::isHalf() const
{
  // for inch mode, there's another "half bit" flag in
  // bit 25 which represents 5/1000
  return (m_packet & 1UL<<25) != 0;
}

