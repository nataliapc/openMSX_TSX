#include "Version.hh"
#include "strCat.hh"

namespace openmsx {

#include "Version.ii"

std::string Version::full()
{
	std::string result = strCat("openMSX ", VERSION);
	if (!RELEASE) strAppend(result, '-', REVISION);
	strAppend(result, " / TSX rev3");
  return result;
}

} // namespace openmsx
