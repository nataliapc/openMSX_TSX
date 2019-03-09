#include "Version.hh"

namespace openmsx {

#include "Version.ii"

std::string Version::full()
{
	return std::string("openMSX ") + VERSION +
	       (RELEASE ? std::string{} : (std::string("-") + REVISION)) +
	       " / TSX rev2.1";
}

} // namespace openmsx
