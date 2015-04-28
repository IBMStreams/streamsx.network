/* Copyright (C) 2013-2015, International Business Machines Corporation */
/* All Rights Reserved */

#ifndef _NETWORK_IPV4_FUNCTIONS_H_
#define _NETWORK_IPV4_FUNCTIONS_H_

// Define SPL types and functions
#include "SPL/Runtime/Function/SPLFunctions.h"

namespace com {
namespace ibm {
namespace streamsx {
namespace network {
namespace ipv4 {
	inline SPL::rstring decToHex(SPL::int64 const & value)
	{
		std::ostringstream out;
		out << std::hex << value;
		return out.str();
	};

	inline SPL::rstring decToOct(SPL::int64 const & value)
	{
		std::ostringstream out;
		out << std::oct << value;
		return out.str();
	};
}
}
}
}
}

#endif  /* _NETWORK_IPV4_FUNCTIONS_H_ */
