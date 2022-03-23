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
// Deallocates dst, reallocates a big enough chunk, and appends them.
void strappend( char** dst, const char* addend );
// -1 if x < y, 1 if x > y, 0 if x == y.
int strcomp( const char* x, const char* y );
// A faster non-lexicographic compare for not null terminated strings.
s32 strSizeComp( const char* a, u64 asize, const char* b, u64 bsize );
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
void printFloatDigits( f64, u64 );
#define printFloat( x ) printFloatDigits( (x), 2 );
// Puts a number in a string, with a trailing nul character, writing at most count bytes.
void intToStringInBase( char* s, u64 n, u64 count, u8 base );
#define intToString( s, n, c ) intToStringInBase( s, n, c, 10 );
void printIntWithPrefix( u64 n, u32 minWidth, char pad );
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth, char pad );
void printArray( u32 indent, u32 numsPerRow, u32 size, const u32* arr );



// Hash tables.
typedef struct bucket{
  u32 hash;
  void* key;
  u64 keySize;
  void* value;
  u64 valueSize;
  u32 index;
} bucket;
typedef struct hasht{
  u32 bits;
  bucket* data;
  u32 size;
  u32* used;
} hasht;
u32 hash( const void* cdata, u32 size, const hasht* ht );
hasht* htNew( void );
void htDestroy( hasht* ht );
// Copies data.
void htAddWithHash( hasht* ht, const void* key, u64 keysize,
		    const void* val, u64 valsize, u32 hash );
#define htAdd( ht, k, ks, v, vs ) htAddWithHash( ht, k, ks, v, vs,\
						 hash( k, ks, ht ) )
#define htAddString( ht, k, v, vs ) htAdd( ht, k, slen( k ), v, vs )
// Returns NULL if key isn't found. If retSize isn't NULL, it is assigned the
// size of the returned array, in bytes.
const void* htFindWithHash( hasht* ht, const void* key, u64 keysize,
			    u64* retSize, u32 hash );
#define htFind( ht, k, ks, rs ) htFindWithHash( ht, k, ks, rs,\
						hash( k, ks, ht ) )
#define htFindString( ht, k, rs ) htFind( ht, k, slen( k ), rs )
// Removes the element key, errors and exits if key isn't found.
void htRemoveWithHash( hasht* ht, const void* key, u64 keysize, u32 hash );
#define htRemove( ht, k, ks ) htRemoveWithHash( ht, k, ks, hash( k, ks, ht ) )
u64 htElementCount( hasht* ht );
const void* htByIndex( hasht* ht, u64 index, u64* retSize );
// Debugging.
void htPrint( hasht* ht );
#ifdef DEBUG
void htTest( void );
#endif
 
// Loads a directory and all it's files into a hash table.
hasht* htLoadDirectory( const char* dirname );
// Serializes a hash table and returns the resulting array. Stores the resulting
// size in size, which must not be NULL. The resulting array must be freed.
const char* htSerialize( const hasht* ht, u64* size );
hasht* htDeserialize( const char* ser );
