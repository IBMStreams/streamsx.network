/*
#######################################################################
# Copyright (C)2015, International Business Machines Corporation and
# others. All Rights Reserved.
#######################################################################
*/

#ifndef _NETWORK_IPV4_FUNCTIONS_H_
#define _NETWORK_IPV4_FUNCTIONS_H_

// Define SPL types and functions
#include "SPL/Runtime/Function/SPLFunctions.h"
#include <arpa/inet.h>

namespace com {
namespace ibm {
namespace streamsx {
namespace networkanalysis {
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

#endif  /* _NETWORK_IPV4_FUNCTIONS_H_ */
