// $Id$

#ifndef __MIDIOUTDEVICE_HH__
#define __MIDIOUTDEVICE_HH__

#include "Pluggable.hh"
#include "SerialDataInterface.hh"

namespace openmsx {

class MidiOutDevice : public Pluggable, public SerialDataInterface
{
public:
	virtual ~MidiOutDevice() = 0;

	// Pluggable (part)
	virtual const std::string& getClass() const;

	// SerialDataInterface (part)
	virtual void setDataBits(DataBits bits);
	virtual void setStopBits(StopBits bits);
	virtual void setParityBit(bool enable, ParityBit parity);
};

} // namespace openmsx

#endif

