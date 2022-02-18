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

// Bit rotation
#define bits( x ) ( sizeof( x ) * 8 )
#define bitMaskForSize( x, b ) ( b & ( bits( x ) - 1 ) )
#define rotl( x, b ) ( ( x << bitMaskForSize( x, b ) ) |\
		       ( x >> ( bits( x ) - bitMaskForSize( x, b ) ) ) )
#define rotr( x, b ) ( ( x >> bitMaskForSize( x, b ) ) |\
		       ( x << ( bits( x ) - bitMaskForSize( x, b ) ) ) )

// String functions.
void strcopy( char* dst, const char* src );
// -1 if x < y, 1 if x > y, 0 if x == y.
int strcomp( const char* x, const char* y );
// True iff str starts with init.
bool strStartsWith( const char* init, const char* str );
void strreverse( char* s );
// Parses at most 17 digits and returns the corresping integer. The provided pointer is moved to the first non-digit char.
u64 parseInt( const char** s );

// Printing functions.
void printRaw( const char* s, u32 size );
void printIntInBase( u64 n, u8 base );
void eprintIntInBase( u64 n, u8 base );
#define printInt( n ) printIntInBase( n, 10 );
#define eprintInt( n ) eprintIntInBase( n, 10 );
void printFloat( f64 );
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void intToStringInBase( char* s, u64 n, u64 count, u8 base );
#define intToString( s, n, c ) intToStringInBase( s, n, c, 10 );
void printIntWithPrefix( u64 n, u32 minWidth, char pad );
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth, char pad );
void printArray( u32 indent, u32 numsPerRow, u32 size, const u32* arr );


// dword aligned arrays. 
typedef char* array;
#define printArr( a ) { printRaw( aData( a ), aSize( a ) ); }
array aNew( u64 size, const char* data );
#define aString( s ) aNew( slen( s ), s )
void aDel( array a );
u64 aSize( array a );
u64 aISize( array a );
char* aData( array a );
u64* aIData( array a );
// -1 if a < b, 1 if a > b, 0 if a == b. This is a not lexicographic, but is
// still a complete ordering. In particular, shorter strings are always lesser.
s32 aComp( array a, array b );
// Buckets.
typedef struct{
  u32 hash;
  array key;
  array value;
  u32 index;
} bucket;
// Hash tables.
typedef struct{
  u32 bits;
  bucket* data;
  u32 size;
  u32* used;
} hasht;
// MAKE SURE TO FRESHLY ALLOCATE WHATEVER YOU PUT IN A HASH TABLE. The
// hash table will not check to see if you insert the same pointer twice.
hasht* htNew( void );
void htDestroy( hasht* ht );
// Takes ownership of the arrays, freeing them in htDestroy.
void htAdd( hasht* ht, array key, array value );
#ifdef DEBUG
void htTest( void );
#endif
