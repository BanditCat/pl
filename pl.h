////////////////////////////////////////////////////////////////////////////////
//     Copyright (c) Jon DuBois 2022. This file is part of pseudoluminal.     //
//                                                                            //
// This program is free software: you can redistribute it and/or modify       //
// it under the terms of the GNU Affero General Public License as published   //
// by the Free Software Foundation, either version 3 of the License, or       //
// (at your option) any later version.                                        //
//                                                                            //
// This program is distributed in the hope that it will be useful,            //
// but WITHOUT ANY WARRANTY; without even the implied warranty of             //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
// GNU Affero General Public License for more details.                        //
//                                                                            //
// You should have received a copy of the GNU Affero General Public License   //
// along with this program.  If not, see <https://www.gnu.org/licenses/>.     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
// Project wide header.                                                       //
////////////////////////////////////////////////////////////////////////////////


// Defines.
#define NAME "Pseudoluminal😄"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define die( x ) { eprint( AT ":!!!!!!!!DEAD!!!!!!!!\n" );\
    eprint( x ); endl(); end( 1 ); }

#define memc { if( !memCheck( 0 ) ){ memCheck( 1 );\
	die( "memory check failed." ); } }
#ifdef DEBUG
#define marc { eprintl( AT ": Marker" ); memc; }
#else
#define marc mark
#endif
#define mark { eprintl( AT ": Marker " ); }

#ifndef NULL
#define NULL 0
#endif

#include <limits.h>



// Types.
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef signed char s8;
typedef signed short int s16;
typedef signed int s32;
typedef signed long long int s64;
typedef float f32;
typedef double f64;

typedef int bool;
#define true 1
#define false 0

typedef void* plvkInstancep;
typedef void* guiStatep;

// Types sanity check.
#if CHAR_BIT != 8
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if INT_MAX != 2147483647 || INT_MIN != -2147483648
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if UINT_MAX != 4294967295 
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if SHRT_MAX != 32767 || SHRT_MIN != -32768 
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if USHRT_MAX != 65535 
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if SCHAR_MAX != 127 || SCHAR_MIN != -128
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if UCHAR_MAX != 255 
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if LLONG_MAX != 9223372036854775807
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 
#if ULLONG_MAX != 18446744073709551615ull
#error Alien build envioronment, check vislib.h for correct typedefs.
#endif 

#define cpi 3.1415926535897932384626433832795028841971693993751058209749445923078

inline u64 slen( const char* str ){
  u64 c = 0;
  while( *( c++ + str ) )
    ;
  return c - 1;
}

typedef struct osstate osstate;

// Global state.
typedef struct {
  void* heap;
  const char** argv;
  int argc;
  // Print fps every second.
  bool fps;
  plvkInstancep vk;
  bool ended;
  void* compressedResources;
  
  // Memory instrumentation.
#ifdef DEBUG
  u64 memallocCount;
  u64 memnextFree;
  u64 memindicesAllocated;
  u64* memindices;
  char** memallocd;
#endif

  //Linear congruential generator.
  u64 seed;

 osstate* osstate;
} states;
extern states state;

