// $Id$

#ifndef __SETETRISDONGLE_HH__
#define __SETETRISDONGLE_HH__

#include "JoystickDevice.hh"

namespace openmsx {

class SETetrisDongle : public JoystickDevice
{
public:
	SETetrisDongle();
	virtual ~SETetrisDongle();

	//Pluggable
	virtual const std::string& getName() const;
	virtual const std::string& getDescription() const;
	virtual void plugHelper(Connector* connector, const EmuTime& time);
	virtual void unplugHelper(const EmuTime& time);

	//JoystickDevice
	virtual byte read(const EmuTime& time);
	virtual void write(byte value, const EmuTime& time);

private:
	byte status;
};

} // namespace openmsx

#endif // __SETETRISDONGLE_HH__
