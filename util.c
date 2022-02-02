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
// Utilities.                                                                 //
////////////////////////////////////////////////////////////////////////////////

#include "pl.h"
#include "os.h"

void strcopy( char* dst, const char* src ){
  u64 c = 0;
  while( src[ c ] ){
    dst[ c ] = src[ c ];
    ++c;
  }
  dst[ c ] = 0;
}
int strcomp( const char* x, const char* y ){
  const char* xp = x;
  const char* yp = y;
  while( 1 ){
    if( !(*xp) ){
      if( !(*yp) )
	return 0;
      else
	return -1;
    }
    if( !(*yp) )
      return 1;
    if( *xp < *yp )
      return -1;
    if( *xp > *yp )
      return 1;
    ++xp;
    ++yp;
  }
}
void strreverse( char* s ){
  u64 e = slen( s ) - 1;
  u64 b = 0;
  while( b < e ){
    char c = s[ e ];
    s[ e ] = s[ b ];
    s[ b ] = c;
    ++b;
    --e;
  }
}
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void intToString( char* s, u64 n, u64 count ){
  u64 i = 0;
  u64 d = n;
  u64 om = count - 1;
  if( !count )
    return;
  if( om ){
    if( !n ){
      s[ 0 ] = '0';
      s[ 1 ] = 0;
      return;
    }
    while( i < om && d ){
      s[ i ] = '0' + d % 10;
      d /= 10;
      ++i;
    }
    s[ i ] = 0;
  }
  strreverse( s );
}
void printInt( u64 n ){
  char s[ 256 ];
  intToString( s, n, 256 );
  print( s );
}
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth ){
  intToString( s, n, count );
  u32 olen = slen( s );
  if( olen < minWidth && ( 1 + minWidth - olen < count ) ){
    u32 shift = minWidth - olen;
    for( s32 i = 0; i < (s32)minWidth; ++i ){
      s32 j = ( minWidth - i ) - 1;
      s32 sj = j - shift;
      if( sj >= 0 )
	s[ j ] = s[ sj ];
      else
	s[ j ] = ' ';
    }
  }
}
void printIntWithPrefix( u64 n, u32 minWidth ){
  char s[ 256 ];
  intToStringWithPrefix( s, n, 256, minWidth );
  print( s );
}
void printArray( u32 indent, u32 numsPerRow, u32 nums, const u32* arr ){
  u32 i = 0;
  u32 r = 0;
  while( i < nums ){
    if( !r ){
      if( i )
	printl( "," );
      for( u32 j = 0; j < indent; ++j )
	print( " " );
      r = 1;
    } else {
      ++r;
      print( ", " );
      if( r == numsPerRow )
	r = 0;
    }
    printIntWithPrefix( arr[ i ], 10 );
    ++i;
    
  }
}
