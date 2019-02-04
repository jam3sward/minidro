#ifndef __Caliper_h
#define __Caliper_h

#include <FastPin.h>

// This class interfaces to cheap digital calipers with serial
// data output via a clock and data pin.
// James Ward 03/02/19
//
class Caliper {
public:
  // Constructor requires the clock and data pin to be specified
  Caliper(byte clockPin, byte dataPin);

  // Read the next available position from the calipers. This
  // function will block until the packet is received. After
  // calling this, the various get functions below can be used
  // to get the current position, units and other flags.
  // This function returns true for success, false in case of
  // failure.
  bool update();

  // Returns the raw 28 bit data packet
  uint32_t getPacket() const;

  // Returns 5 digits in BCD format
  uint32_t getDigitsBCD() const;

  // Returns true if the caliper value is negative
  bool isNegative() const;

  // Returns true if the calipers are in metric mode
  bool isMetric() const;

  // Returns true if the calipers are in inch mode
  bool isInch() const;

  // Returns true if the half flag is set in inch mode
  bool isHalf() const;

private:
  FastPin   m_clockPin; ///< The clock pin (input)
  FastPin   m_dataPin;  ///< The data pin (input)
  uint32_t  m_packet;   ///< The last packet we received
};

#endif//__Caliper_h

