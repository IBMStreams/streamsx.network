/*
** Copyright (C) 2018  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef GEOHASH_FUNCTIONS_H_
#define GEOHASH_FUNCTIONS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>

#include "SPL/Runtime/Function/SPLFunctions.h"

namespace com { namespace ibm { namespace streamsx { namespace network { namespace geohash {

          //~~~~~~~ based on https://github.com/simplegeo/libgeohash/blob/master/geohash.c ~~~~~~~~~
          
          typedef struct { // measured in in degrees
            double high;
            double low;
          } Interval;
                    
          static const char* characters = "0123456789bcdefghjkmnpqrstuvwxyz";

          /**
           * The geohashEncode() function encodes a latitude/longitude pair as a
           * 'geohash' of a specified precision.
           */

          static SPL::rstring geohashEncode(SPL::float64 latitude, SPL::float64 longitude, SPL::int32 precision) {
            
            // clamp the range of the parameters
            if (latitude<-90 || latitude>+90) return SPL::rstring();
            if (longitude<-180 || longitude>+180) return SPL::rstring();
            if (precision<1 || precision>12) return SPL::rstring();
            
            // encode the latitude/longitude by progressively narrowing the range of their geocode
            Interval latitudeInterval = { +90, -90 };
            Interval longitudeInterval = { +180, -180 };            
            bool isEven = true;
            unsigned int characterIndex = 0;
            
            // allocate the geohash and prefill it with '?' characters
            SPL::rstring geohash(precision, '?');
            
            // encode the latitude/longitude one bit at a time to the specified precision
            for(int i = 1; i<=5*precision; i++) {
              
              Interval *interval;
              double coordinate;

              if (isEven) {
                interval = &longitudeInterval;
                coordinate = longitude;                
              } else {
                interval = &latitudeInterval;
                coordinate = latitude;   
              }
              
              double midpoint = (interval->low + interval->high) / 2.0;
              characterIndex = characterIndex << 1;
              
              if (coordinate > midpoint) {
                interval->low = midpoint;
                characterIndex |= 0x01;
              } else {
                interval->high = midpoint;
              }
              
              if ( !(i % 5) ) {
                geohash[ (i-1) / 5 ] = characters[characterIndex];
                characterIndex = 0;
              }
              
              isEven = !isEven;
            }
            
            // return the encoded geohash
            return geohash;
          }

          /**
           * The geohashDecode() function decodes a 'geohash' as the latitude
           * and longitude of its center point and bounding box. The argument
           * value is a string of 1 to 12 characters composed of the characters
           * '0123456789bcdefghjkmnpqrstuvwxyz'.
           */

          static SPL::list<SPL::float64> geohashDecode(SPL::rstring geohash) {
            
            // geohash codes are 1 to 12 characters long
            if (geohash.length()<1) return SPL::list<SPL::float64>();
            if (geohash.length()>12) return SPL::list<SPL::float64>();

            // decode the geohash by progressively narrowing the range of its latitude/longitude
            bool isEven = true;
            Interval latitudeInterval = { +90, -90 };
            Interval longitudeInterval = { +180, -180 };
            
            // decode the geohash one character at a time
            for (int i = 0; i < geohash.length(); i++) {
              
              // get the index of the next character in the geohash
              const char* k = strchr(characters, geohash[i]);
              if (!k) return SPL::list<SPL::float64>();         
              unsigned int characterIndex = k - characters;
              
              // decode the character by narrowing the latitude or longitude range, one bit at a time
              for (int j = 0; j<5; j++) {
                
                Interval* interval = isEven ? &longitudeInterval : &latitudeInterval;
                
                double delta = ( interval->high - interval->low ) / 2.0;
                
                if ((characterIndex << j) & 0x0010) 
                  interval->low += delta;
                else
                  interval->high -= delta;
                
                isEven = !isEven;
              }
            }
            
            // assign the coordinates of the center and edges of the geohash
            SPL::list<SPL::float64> coordinates(6, 0.0);
            coordinates[0] = latitudeInterval.high - ((latitudeInterval.high - latitudeInterval.low) / 2.0); // center point
            coordinates[1] = longitudeInterval.high - ((longitudeInterval.high - longitudeInterval.low) / 2.0); // center point
            coordinates[2] = latitudeInterval.high; // top/northern edge
            coordinates[3] = longitudeInterval.high; // right/eastern edge
            coordinates[4] = latitudeInterval.low; // bottom/southern edge
            coordinates[5] = longitudeInterval.low; // left/western edge
            
            // return the coordinates of the geohash as a list of six floats
            return coordinates;
          }
          

} } } } }

#endif /* GEOHASH_FUNCTIONS_H_ */
