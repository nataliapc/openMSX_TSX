#ifndef MSXRS232_HH
#define MSXRS232_HH

#include "MSXDevice.hh"
#include "IRQHelper.hh"
#include "RS232Connector.hh"
#include "I8251.hh"
#include "I8254.hh"
#include <memory>

namespace openmsx {

class Ram;
class Rom;
class BooleanSetting;

class MSXRS232 final : public MSXDevice, public RS232Connector
{
public:
	explicit MSXRS232(const DeviceConfig& config);
	~MSXRS232();

	void powerUp(EmuTime::param time) override;
	void reset(EmuTime::param time) override;
	byte readIO(word port, EmuTime::param time) override;
	byte peekIO(word port, EmuTime::param time) const override;
	void writeIO(word port, byte value, EmuTime::param time) override;
	byte readMem(word address, EmuTime::param time) override;
	// TODO: implement peekMem, because the default isn't OK anymore
	const byte *getReadCacheLine(word start) const override;
	void writeMem(word address, byte value, EmuTime::param time) override;
	byte* getWriteCacheLine(word start) const override;

	// RS232Connector  (input)
	void setDataBits(DataBits bits) override;
	void setStopBits(StopBits bits) override;
	void setParityBit(bool enable, ParityBit parity) override;
	void recvByte(byte value, EmuTime::param time) override;
	bool ready() override;
	bool acceptsData() override;

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	byte readIOImpl(word port, EmuTime::param time);
	void writeIOImpl(word port, byte value, EmuTime::param time);

	byte readStatus(EmuTime::param time);
	void setIRQMask(byte value);
	void setRxRDYIRQ(bool status);
	void enableRxRDYIRQ(bool enabled);

	struct Counter0 final : ClockPinListener {
		void signal(ClockPin& pin, EmuTime::param time) override;
		void signalPosEdge(ClockPin& pin, EmuTime::param time) override;
	} cntr0; // counter 0 rx clock pin

	struct Counter1 final : ClockPinListener {
		void signal(ClockPin& pin, EmuTime::param time) override;
		void signalPosEdge(ClockPin& pin, EmuTime::param time) override;
	} cntr1; // counter 1 tx clock pin

	I8254 i8254;

	struct I8251Interf final : I8251Interface {
		void setRxRDY(bool status, EmuTime::param time) override;
		void setDTR(bool status, EmuTime::param time) override;
		void setRTS(bool status, EmuTime::param time) override;
		bool getDSR(EmuTime::param time) override;
		bool getCTS(EmuTime::param time) override;
		void setDataBits(DataBits bits) override;
		void setStopBits(StopBits bits) override;
		void setParityBit(bool enable, ParityBit parity) override;
		void recvByte(byte value, EmuTime::param time) override;
		void signal(EmuTime::param time) override;
	} interf;

	I8251 i8251;
	const std::unique_ptr<Rom> rom; // can be nullptr
	const std::unique_ptr<Ram> ram; // can be nullptr

	IRQHelper rxrdyIRQ;
	bool rxrdyIRQlatch;
	bool rxrdyIRQenabled;

	const bool hasMemoryBasedIo;
	bool ioAccessEnabled;

	const std::unique_ptr<BooleanSetting> switchSetting; // can be nullptr
};
SERIALIZE_CLASS_VERSION(MSXRS232, 2);


} // namespace openmsx

#endif
