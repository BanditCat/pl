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
#include "util.h"

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
bool strStartsWith( const char* init, const char* str ){
  const char* xp = init;
  const char* yp = str;
  while( 1 ){
    if( !(*xp) )
      return 1;
    if( !(*yp) )
      return 0;
    if( *xp != *yp )
      return 0;
    ++xp;
    ++yp;
  }
  
}
u64 parseInt( const char** s ){
  u64 ret = 0;
  u64 digits = 0;
  while( digits < 17 && **s >= '0' && **s <= '9' ){
    ret *= 10;
    ret += **s - '0';
    ++*s;
    ++digits;
  }
  return ret;
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
void printFloat( f64 f ){
  if( f < 0 ){
    print( "-" );
    f *= -1.0;
  }
  if( f < 0.00000000001 )
    print( "0.0" );
  else if( f > 9223372036854775806.0 )
    print( "<large>" );
  else{
    u64 integerPart = f;
    f = f - integerPart;
    f *= 100000000000;
    u64 fractionalPart = f;
    printInt( integerPart );
    print( "." );
    printIntWithPrefix( fractionalPart, 11, '0' );
  }
}
void intToStringWithPrefix( char* s, u64 n, u64 count, u32 minWidth, char pad ){
  intToString( s, n, count );
  u32 olen = slen( s );
  if( olen < minWidth && ( 1 + minWidth - olen ) < count ){
    u32 shift = minWidth - olen;
    for( s32 i = 0; i < (s32)minWidth; ++i ){
      s32 j = ( minWidth - i ) - 1;
      s32 sj = j - shift;
      if( sj >= 0 )
	s[ j ] = s[ sj ];
      else
	s[ j ] = pad;
    }
    s[ minWidth ] = 0;
  }
}
void printIntWithPrefix( u64 n, u32 minWidth, char pad ){
  char s[ 256 ];
  intToStringWithPrefix( s, n, 256, minWidth, pad );
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
    printIntWithPrefix( arr[ i ], 10, ' ' );
    ++i;
    
  }
}


// Hash tables.
#define HASHTABLE_INITIAL_SIZE_IN_BITS 8
#define HASHTABLE_MAX_PROBES 10
#define HASH_P 2305843009213693951
#define HASH_P_BITS 61
#define HASH_I 5381
u32 hash( array a, u32 size ){
  u32 asize = aSize( a );
  u32* data = aIData( a );
  u64 h = HASH_I;
  for( u32 i = 0; i < asize; i += 4 ){
    // h << 5 + h = h times 33
    h = ( h << 5 ) + h + *data;
    ++data;
    // h mod p, p a Mersenne prime
    h = ( h & HASH_P ) + ( h >> HASH_P_BITS );
    if( h >= HASH_P )
      h -= HASH_P;
  }
  // Rotate bits.
  u32 shift = size - HASHTABLE_INITIAL_SIZE_IN_BITS;
  u32 h32 = h;
  return ( h32 << shift ) | ( h32 >> ( 32 - shift ) );
}
array aNew( u32 size, const char* data ){
  u32 asize = size + ( ( 4 - ( size & 3 ) ) & 3 );
  newa( ret, char, asize + 4 );
  memcpy( ret + 4, data, size );
  *((u32*)ret) = size;
  return ret;  
}
void aDel( array a ){
  memfree( a );
}
u32 aSize( array a ){
  return  *((u32*)a);
}
char* aData( array a ){
  return ((char*)a) + 4;
}
u32* aIData( array a ){
  return ((u32*)a) + 1;
}
hasht* htNew( void ){
  new( ret, hasht );
  ret->bits = HASHTABLE_INITIAL_SIZE_IN_BITS;
  ret->data = newae( bucket, 1 << HASHTABLE_INITIAL_SIZE_IN_BITS );
  ret->used = newae( u32, 1 << HASHTABLE_INITIAL_SIZE_IN_BITS );
  ret->size = 0;
  return ret;
}
void htDestroy( hasht* ht ){
  for( u32 i = 0; i < ht->size; ++i ){
    memfree( ht->data[ ht->used[ i ] ].key );
    memfree( ht->data[ ht->used[ i ] ].value );
  }
  memfree( ht->data );
  memfree( ht->used );
  memfree( ht );
}
void htRehash( hasht* ht ){
  ++ht->bits;
  u32 mask = ( 1 << ht->bits ) - 1;
  newa( nd, bucket, 1 << ht->bits );
  newa( nu, u32, 1 << ht->bits );
  for( u32 i = 0; i < ht->size; ++i ){
    bucket* item = ht->data + ht->used[ i ];
    u32 nh = ( item->hash << 1 ) | ( item->hash >> 31 );
    nu[ i ] = nh & mask;
    nd[ nh & mask ].hash = nh;
    nd[ nh & mask ].key = item->key;
    nd[ nh & mask ].value = item->value;
  }
  memfree( ht->data );
  ht->data = nd;
  memfree( ht->used );
  ht->used = nu;
}
void htAdd( hasht* ht, array key, array value ){
  u32 h, probes;
  u32 mask = ( 1 << ht->bits ) - 1;
  h = hash( key, ht->bits );
  while( 1 ){
    probes = 0;
    while( ht->data[ h ].key && probes < HASHTABLE_MAX_PROBES ){
      h = ( h + 1 ) & mask;
      ++probes;
    }
    if( probes >= HASHTABLE_MAX_PROBES ){
      htRehash( ht );
    } else
      break;
  }
  ht->data[ h & mask ].hash = h;
  ht->data[ h & mask ].key = key;
  ht->data[ h & mask ].key = value;
  ht->used[ ht->size++ ] = h & mask;
}
