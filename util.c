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

void memcopy( const char* src, char* dst, u64 count ){
  for( u64 i = 0; i < count; ++i )
    dst[ i ] = src[ i ];
}
void strcopy( const char* src, char* dst ){
  u64 c = 0;
  while( src[ c ] ){
    dst[ c ] = src[ c ];
    ++c;
  }
  dst[ c ] = 0;
}
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void tostring( char* s, u64 n, u64 count );
