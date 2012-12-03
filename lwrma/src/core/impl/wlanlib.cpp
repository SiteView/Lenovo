#include "wlanlib.h"

WlanLib::WlanLib()
	: m_libWlanApi(QLatin1String("wlanapi"))
{
}

WlanLib::~WlanLib()
{
	m_libWlanApi.unload();
}

bool WlanLib::init()
{
	if (!m_libWlanApi.load()) {
		return false;
	}

	bool allResolved = true;

#undef WLANAPI_ENTRY
#define WLANAPI_ENTRY(r, n, p) if ((n = reinterpret_cast<PFN_ ## n>(m_libWlanApi.resolve(#n))) == 0) allResolved = false;
#include "wlandef.h"
#undef WLANAPI_ENTRY

	if (!allResolved) {
		return false;
	}

	return true;
}
