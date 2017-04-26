/*
** Copyright (C) 2017  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef MISC_NETWORK_FUNCTIONS_H_
#define MISC_NETWORK_FUNCTIONS_H_

#include "SPL/Runtime/Function/SPLFunctions.h"

#define ESC_CHAR '\\' 
namespace com { namespace ibm { namespace streamsx { namespace network { namespace misc {

	// TODO: Use destLen to check bounds
	static inline __attribute__((always_inline))
		size_t moveEscaped( char* dest, const int32_t destLen, const uint8_t* source, const size_t srcLen,
					const uint8_t delim1 = 0, const uint8_t delim2 = 0, const uint8_t delim3 = 0) {
			uint32_t i, offset = 0;
			uint8_t  uNibble, lNibble;
			size_t   bytes_formatted = 0;
			for(i = 0; i < srcLen; ++i) {
				uint8_t ch = source[i]; 
				if((ch < 32) || (ch > 126) || (ch == ESC_CHAR) ||
				   (ch == delim1) || (ch == delim2) || (ch == delim3)) {
					dest[offset++] = ESC_CHAR;
					// dest[offset++] = 'x';  // Might be nice to use standard C style escaping ...
					uNibble = ((ch >> 4) & 0xF); 
					lNibble = (ch & 0xF); 
					dest[offset++] = (uNibble <= 9) ? '0' + uNibble : 'A' - 10 + uNibble; 
					dest[offset++] = (lNibble <= 9) ? '0' + lNibble : 'A' - 10 + lNibble; 
					bytes_formatted += 3;
				} else {
					dest[offset++] = ch;
					bytes_formatted++;
				}
			}
			return bytes_formatted;
		}

	static SPL::rstring stringEscape(SPL::rstring inputString, const uint8_t delim1 = 0, const uint8_t delim2 = 0, const uint8_t delim3 = 0) {
		const uint8_t *srcData = (uint8_t *)inputString.data();
		uint16_t srcSize = inputString.size(); 
		char dstBuffer[4096];
		size_t cnt = moveEscaped(dstBuffer, 4096, srcData, srcSize, delim1, delim2, delim3);
		return SPL::rstring(dstBuffer, cnt); 
	}

        static SPL::list<SPL::rstring> stringEscape(SPL::list<SPL::rstring> inputString, const uint8_t delim1 = 0, const uint8_t delim2 = 0, const uint8_t delim3 = 0) {
                SPL::list<SPL::rstring> strings;
                SPL::list<SPL::rstring>::const_iterator it;
                for(it = inputString.begin(); it != inputString.end(); ++it) {
                        const uint8_t *srcData = (uint8_t *)((*it).data());
                        uint16_t srcSize = (*it).size();
                        char dstBuffer[4096];
                        size_t cnt = moveEscaped(dstBuffer, 4096, srcData, srcSize, delim1, delim2, delim3);
                        strings.push_back(SPL::rstring(dstBuffer, cnt));
                }

                return strings;
        }

        static SPL::rstring stringEscapeConcat(SPL::list<SPL::rstring> inputString,
                                               const uint8_t subfieldDelim,
                                               const uint8_t delim1 = 0,
                                               const uint8_t delim2 = 0,
                                               const uint8_t delim3 = 0) {
                SPL::rstring stringOut;
                SPL::list<SPL::rstring>::const_iterator it;
                char dstBuffer[4096];
                uint32_t dstOffset = 0;
                SPL::boolean isFirst = true;
                for(it = inputString.begin(); it != inputString.end(); ++it) {
                        const uint8_t *srcData = (uint8_t *)((*it).data());
                        uint16_t srcSize = (*it).size();
                        if(isFirst) {
                                isFirst = false;
                        } else {
                                if(subfieldDelim) {
                                       dstBuffer[dstOffset++] = subfieldDelim;
                                }
                        }

                        size_t cnt = moveEscaped(&dstBuffer[dstOffset], 4096, srcData, srcSize,
                                                 delim1, delim2, delim3);
                        dstOffset += cnt;
                }

                return SPL::rstring(dstBuffer, dstOffset);
        }

} } } } }

#endif /* MISC_NETWORK_FUNCTIONS_H_ */
