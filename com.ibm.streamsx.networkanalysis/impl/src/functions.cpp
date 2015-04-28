#include <functions.h>

namespace com {
namespace ibm {
namespace streamsx {
namespace network {
namespace utils {
	SPL::rstring decToHex(SPL::int64 const & value)
	{
		std::ostringstream out;
		out << std::hex << value;
		return out.str();
	}

	SPL::rstring decToOct(SPL::int64 const & value)
	{
		std::ostringstream out;
		out << std::oct << value;
		return out.str();
	}
}
}
}
}
}
