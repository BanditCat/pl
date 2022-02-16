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

// String functions.
void strcopy( char* dst, const char* src );
// -1 if x < y, 1 if x > y, 0 if x == y.
int strcomp( const char* x, const char* y );
// True iff str starts with init.
bool strStartsWith( const char* init, const char* str );
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void intToString( char* s, u64 n, u64 count );
void strreverse( char* s );
// Parses at most 17 digits and returns the corresping integer. The provided pointer is moved to the first non-digit char.
u64 parseInt( const char** s );

// Printing functions.
void printInt( u64 n );
void printFloat( f64 );
void printIntWithPrefix( u64 n, u32 minWidth, char pad );
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth, char pad );
void printArray( u32 indent, u32 numsPerRow, u32 nums, const u32* arr );


// Arrays.
typedef char* array;
array aNew( u32 size, const char* data );
void aDel( array a );
u32 aSize( array a );
char* aData( array a );
u32* aIData( array a );
// Buckets.
typedef struct{
  u32 hash;
  array key;
  array value;
} bucket;
// Hash table size is in bits.
typedef struct{
  u32 bits;
  bucket* data;
  u32 size;
  u32* used;
} hasht;

hasht* htNew( void );
void htDestroy( hasht* ht );
// Takes ownership of the arrays, freeing them in htDestroy.
void htAdd( hasht* ht, array key, array value );
