// $Id$

#ifndef __MSXMIDI_HH__
#define __MSXMIDI_HH__

#include "MSXDevice.hh"
#include "I8251.hh"
#include "I8254.hh"
#include "IRQHelper.hh"
#include "MidiInConnector.hh"
#include "MidiOutConnector.hh"

namespace openmsx {

class ClockPin;

class MSXMidi: public MSXDevice, public MidiInConnector
{
public:
	MSXMidi(const XMLElement& config, const EmuTime& time);
	virtual ~MSXMidi();

	virtual void reset(const EmuTime& time);
	virtual byte readIO(byte port, const EmuTime& time);
	virtual byte peekIO(byte port, const EmuTime& time) const;
	virtual void writeIO(byte port, byte value, const EmuTime& time);

	// MidiInConnector
	virtual bool ready();
	virtual bool acceptsData();
	virtual void setDataBits(DataBits bits);
	virtual void setStopBits(StopBits bits);
	virtual void setParityBit(bool enable, ParityBit parity);
	virtual void recvByte(byte value, const EmuTime& time);

private:
	void setTimerIRQ(bool status, const EmuTime& time);
	void enableTimerIRQ(bool enabled, const EmuTime& time);
	void updateEdgeEvents(const EmuTime& time);
	void setRxRDYIRQ(bool status);
	void enableRxRDYIRQ(bool enabled);

	bool timerIRQlatch;
	bool timerIRQenabled;
	IRQHelper timerIRQ;
	bool rxrdyIRQlatch;
	bool rxrdyIRQenabled;
	IRQHelper rxrdyIRQ;

	// counter 0 clock pin
	class Counter0 : public ClockPinListener {
	public:
		Counter0(MSXMidi& midi);
		virtual ~Counter0();
		virtual void signal(ClockPin& pin,
					const EmuTime& time);
		virtual void signalPosEdge(ClockPin& pin,
					const EmuTime& time);
	private:
		MSXMidi& midi;
	} cntr0;
	
	// counter 2 clock pin
	class Counter2 : public ClockPinListener {
	public:
		Counter2(MSXMidi& midi);
		virtual ~Counter2();
		virtual void signal(ClockPin& pin,
					const EmuTime& time);
		virtual void signalPosEdge(ClockPin& pin,
					const EmuTime& time);
	private:
		MSXMidi& midi;
	} cntr2;
	
	I8254 i8254;

	// I8251Interface
	class I8251Interf : public I8251Interface {
	public:
		I8251Interf(MSXMidi& midi);
		virtual ~I8251Interf();
		virtual void setRxRDY(bool status, const EmuTime& time);
		virtual void setDTR(bool status, const EmuTime& time);
		virtual void setRTS(bool status, const EmuTime& time);
		virtual bool getDSR(const EmuTime& time);
		virtual bool getCTS(const EmuTime& time);
		virtual void setDataBits(DataBits bits);
		virtual void setStopBits(StopBits bits);
		virtual void setParityBit(bool enable, ParityBit parity);
		virtual void recvByte(byte value, const EmuTime& time);
		virtual void signal(const EmuTime& time);
	private:
		MSXMidi& midi;
	} interf;
	
	I8251 i8251;
	MidiOutConnector outConnector;
};

} // namespace openmsx

#endif
