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


#define memcopy( d, s, t, c ) memcpy( d, s, sizeof( t ) * c )
void strcopy( void* dst, const void* src );
// -1 if x < y, 1 if x > y, 0 if x == y.
int strcomp( const char* x, const char* y );
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void intToString( char* s, u64 n, u64 count );
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void printInt( u64 n );
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth );
void printIntWithPrefix( u64 n, u32 minWidth );
void strreverse( char* s );
void printArray( u32 indent, u32 numsPerRow, u32 nums, const u32* arr );
