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
void printRaw( const char* sa, u32 size ){
  const u8* s = (const u8*)sa;
  char pa[ 2 ] = {};
  bool raw = 0;
  for( u32 i = 0; i < size; ++i ){
    bool oraw = raw;
    if( s[ i ] > 32 && s[ i ] < 127 && s[ i ] != '|' )
      raw = 0;
    else
      raw = 1;
    if( oraw ^ raw )
      print( "|" );
    if( raw ){
      if( s[ i ] < 16 )
	print( "0" );
      printIntInBase( s[ i ], 16 );
    }else{
      pa[ 0 ] = s[ i ];
      print( pa );
    }
  }      
  if( raw )
    print( "|" );
}
void strcopy( char* dst, const char* src ){
  u64 c = 0;
  while( src[ c ] ){
    dst[ c ] = src[ c ];
    ++c;
  }
  dst[ c ] = 0;
}
void strappend( char** dst, const char* addend ){
  u64 dlen = slen( *dst );
  u64 alen = slen( addend );
  newa( ret, char, dlen + alen + 1 );
  for( u64 i = 0; i < alen + dlen; ++i ){
    if( i < dlen ){
      ret[ i ] = (*dst)[ i ];
    }else{
      ret[ i ] = addend[ i - dlen ];
    }
  }
  ret[ dlen + alen ] = 0;
  memfree( *dst );
  *dst = ret;
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
void intToStringInBase( char* s, u64 n, u64 count, u8 base ){
  static const char numerals[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  static const u32 nlen = sizeof( numerals );
  if( base > nlen )
    die( "Invalid base specified in intToStringInBase." );
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
      s[ i ] = numerals[ d % base ];
      d /= base;
      ++i;
    }
    s[ i ] = 0;
  }
  strreverse( s );
}
void printIntInBase( u64 n, u8 base ){
  char s[ 256 ];
  intToStringInBase( s, n, 256, base );
  print( s );
}
void eprintIntInBase( u64 n, u8 base ){
  char s[ 256 ];
  intToStringInBase( s, n, 256, base );
  eprint( s );
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
void printArray( u32 indent, u32 numsPerRow, u32 size, const u32* arr ){
  u32 i = 0;
  u32 r = 0;
  while( i < size ){
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
#define HASHTABLE_INITIAL_SIZE_IN_BITS 3
#define HASHTABLE_MAX_PROBES 5
#define HASH_P 2305843009213693951
#define HASH_P_BITS 61
#define HASH_I 538184554674741098
#define HASH_TABLE_SHIFT 1

// a "good" hash function.
u32 hash( const char* cdata, u32 size, const hasht* ht ){
  const char* data = cdata;
  u64 h = HASH_I;
  for( u64 i = 0; i < size; ++i ){
    // h << 5 + h = h times 33
    h = ( h << 5 ) + h + *data;
    ++data;
    // h mod p, p a Mersenne prime
    h = ( h & HASH_P ) + ( h >> HASH_P_BITS );
    if( h >= HASH_P )
      h -= HASH_P;
  }
  // Rotate bits.
  u32 shift = ht->bits * HASH_TABLE_SHIFT - HASHTABLE_INITIAL_SIZE_IN_BITS;
  u32 h32 = h;
  return rotl( h32, shift );
}

s32 strSizeComp( const char* a, u64 asize, const char* b, u64 bsize ){
  if( asize < bsize )
    return -1;
  if( asize > bsize )
    return 1;
  const char* astart = a;
  const char* aend = astart + asize - 1;
  const char* bstart = b;
  const char* bend = bstart + bsize - 1;
  while( astart <= aend ){
    if( *astart < *bstart || *aend < *bend )
      return -1;
    if( *astart > *bstart || *aend > *bend )
      return 1;
    ++astart; ++bstart;
    --aend; --bend;
  }
  return 0;
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
    u32 nh = rotl( item->hash, HASH_TABLE_SHIFT );
    u32 index = nh & mask;
    while( nd[ index ].key )
      index = ( index + 1 ) & mask;
    nu[ i ] = index;
    nd[ index ].hash = nh;
    nd[ index ].key = item->key;
    nd[ index ].keySize = item->keySize;
    nd[ index ].value = item->value;
    nd[ index ].valueSize = item->valueSize;
    nd[ index ].index = i;
  }
  memfree( ht->data );
  ht->data = nd;
  memfree( ht->used );
  ht->used = nu;
}
void htAddWithHash( hasht* ht, const char* key, u64 keySize,
		    const char* val, u64 valsize, u32 h ){
  u32 probes = 0;
  u32 mask = ( 1 << ht->bits ) - 1;
  u32 index = h & mask;
  while( 1 ){
    while( ht->data[ index ].key && probes < HASHTABLE_MAX_PROBES ){
      if( ht->data[ index ].hash == h &&
	  !strSizeComp( ht->data[ index ].key, ht->data[ index ].keySize,
			key, keySize ) ){
	die( "Duplicate key in htAdd." );
      }
      index = ( index + 1 ) & mask;
      ++probes;
    }
    if( probes >= HASHTABLE_MAX_PROBES ){
      htRehash( ht );
      htAdd( ht, key, keySize, val, valsize );
      return;
    } else
      break;
  }
  ht->data[ index ].hash = h;
  ht->data[ index ].key = copy( key, keySize );
  ht->data[ index ].keySize = keySize;
  ht->data[ index ].value = copy( val, valsize );
  ht->data[ index ].valueSize = valsize;
  ht->data[ index ].index = ht->size;
  ht->used[ ht->size++ ] = index;
}
const char* htFindWithHash( hasht* ht, const char* key, u64 keySize,
			    u64* retSize, u32 h ){ 
  u32 mask = ( 1 << ht->bits ) - 1;
  u32 index = h & mask;
  u32 probes = 0;
  while( ht->data[ index ].key && probes <= ht->size ){
    if( ht->data[ index ].hash == h &&
	!strSizeComp( ht->data[ index ].key, ht->data[ index ].keySize,
		      key, keySize ) ){
      if( retSize )
	*retSize = ht->data[ index ].valueSize;
      return ht->data[ index ].value;
    }
    index = ( index + 1 ) & mask;
    ++probes;
  }
  return NULL;
}
void htRemoveWithHash( hasht* ht, const char* key, u64 keySize, u32 h ){
  u32 mask = ( 1 << ht->bits ) - 1;
  u32 index = h & mask;
  u32 probes = 0;
  while( ht->data[ index ].key && probes <= ht->size ){
    if( ht->data[ index ].hash == h &&
	!strSizeComp( ht->data[ index ].key, ht->data[ index ].keySize,
		      key, keySize ) ){
      ht->data[ index ].hash = 0;
      memfree( ht->data[ index ].key );
      ht->data[ index ].key = 0;
      ht->data[ index ].keySize = 0;
      memfree( ht->data[ index ].value );
      ht->data[ index ].value = 0;
      ht->data[ index ].valueSize = 0;
      --ht->size;
      ht->data[ ht->used[ ht->size ] ].index = ht->data[ index ].index;
      ht->used[ ht->data[ index ].index ] = ht->used[ ht->size ];
      ht->used[ ht->size ] = 0;
      ht->data[ index ].index = 0;
      return;
    }
    index = ( index + 1 ) & mask;
    ++probes;
  }
  die( "Key not found in htRemove." );
}

		   
void htPrint( hasht* ht ){
  printl( "Hash table{" );
  print( "  size: " ); printInt( ht->size ); endl();
  print( "  bits: " ); printInt( ht->bits ); endl();
  printl( "  used indices:" );
  printArray( 2, 4, ht->size, ht->used ); endl();
  for( u32 i = 0; i < ( 1 << ht->bits  ); ++i ){
    bucket* cb = ht->data + i;
    if( cb->key ){
      print( "  bucket " ); printInt( i ); printl( ":" );
      print( "    hash: " ); printIntInBase( cb->hash, 2 ); endl();
      print( "    key: <" ); printInt( cb->keySize ); print( ">" );
      printRaw( cb->key, cb->keySize ); endl();
      print( "    val: <" ); printInt( cb->valueSize ); print( ">" );
      if( cb->valueSize < 100 ){
	printRaw( cb->value, cb->valueSize );
      }
       endl();
      print( "    index: " ); printInt( cb->index ); endl();
    }
  }
  printl( "}" );
}
#ifdef DEBUG
#define NUM_TEST_STRINGS ( sizeof( testStrings ) / sizeof( testStrings[ 0 ] ) )
const char* testStrings[] = {
  "aLETkG7FTz",
  "gOuSpSzmV8",
  "PrfmCpqyBl",
  "bVKLvBmcDA",
  "MMh23XOmyc",
  "i6OFEvApiJ",
  "Ua3gCtOCEY",
  "j0Utjq8caB",
  "DeGEiJIojG",
  "v0YKwMYFDI",
  "mUvXMD5Ubb",
  "iF3Is3NCD9",
  "",
  "",
  "a",
  "",
  "",
  "8tEaPQya9S",
  "EfbItr53iR",
  "femoMs9hI6",
  "BdhB0cgOmK",
  "L44ZRKfX1j",
  "etke5CTrwk",
  "GyOX7C8Mwv",
  "r7Mdd3Ck7r",
  "R91NQkDqIz",
  "gbctCU1mXa",
  "Q4YzlvDF0H",
  "cV1omg6YZl",
  "DawNND6rN9",
  "H6yTuIZzp1",
  "lBgXVddeYH",
  "QjBp1SFcL0",
  "SnirrcAcFO",
  "pgWtFKELCq",
  "szasAK5AII",
  "zdITxsmCrn",
  "HhHlSp4hkx",
  "LmoKL5rT79",
  "E0CAiTC8Oe",
  "fiaHOkdHT7",
  "HdRUxs2KgY",
  "uMiFKD1aSm",
  "lHm0uplF2V",
  "dScYsWRTty",
  "UwLoAzQX0x",
  "P7azyjCD00",
  "mTB7kBSyJs",
  "BIKIYtUfzi",
  "s1vOSkMo0e",
  "mqz7RqPwl7",
  "qOTYRy3CWz",
  "rE6wwJaEbV",
  "s48r13c1AR",
  "VTjIIwCYGJ",
  "Xyz7Cgf4uA",
  "g4iJDzl03B",
  "bNNWi8AG2R",
  "5S6EKNBeXe",
  "T9K03Pt1Lq",
  "nNEvEAtEs6",
  "lZdesJgUND",
  "S7rfsTQMiS",
  "Cai84ndY5z",
  "YS88dEtmo6",
  "ucUmmrMRSt",
  "hfi0kZizz4",
  "D2VK0s4G2k",
  "5SANa2PNc6",
  "u761IgRIN9",
  "aK0QUJisuA",
  "IN8uyvZ53C",
  "CCkH9WJWep",
  "eggn4LWIkU",
  "hxQDhmoxg3",
  "3Togy6aFV8",
  "kqMe66nZAW",
  "Pd1HzW1OJ7",
  "kJ20gf3UaB",
  "1rSWb1cRZ1",
  "sWkbNIXMxm",
  "ikgMrPTW3g",
  "bdToqgnBj6",
  "u6bYJh9srP",
  "yHBYWOoiaH",
  "Z6E81nbz7G",
  "secEsne6hV",
  "v3aX32E6JL",
  "mbeiglpEyM",
  "Zsdd1kwhB8",
  "DkkMwHLuq0",
  "uGxAq2CRM2",
  "jKg85eWdcz",
  "ImGRGalMwp",
  "yKKmL0Zsd7",
  "cyefQt3MO7",
  "NVOJ4MNmCY",
  "DboEviiWXR",
  "SHjYZ7arP4",
  "pBTg5mXc1l",
  "nZ5GG6owiO",
  "ZDseLhZIim",
  "QfYIfSoWax",
  "t4IZdozvqQ",
  "Tdwq6DqGrI",
  "9aYhTFqtOR",
  "94B8Wi5k7v",
  "MbHpyd4xUC",
  "r0708V6O6q",
  "ECM1Jegd5c",
  "A14dqgxMLB",
  "BbHqq49Gzw",
  "UCvMnUtTSI",
  "r5eMsnARO7",
  "yTRlR7sqzj",
  "c6AFtk7Vun",
  "tnBGUmYZDI",
  "snuzOXxczl",
  "CzjU4swsI8",
  "wPpucyKDbN",
  "HD3vMP3MbS",
  "hxdx9k6ATj",
  "SwTYHdMcVO",
  "vQ7AAfe84T",
  "ljwgPcfJpA",
  "",
  "",
  "",
  "",
  "",
  "5yvpVFP9Ey",
  "OrfuxwU",
  "Z5pYrIx90m",
  "0ghN4tcjQz",
  "Y3YP3pVlwP",
  "h7gqwSttpc",
  "tUYkG47fik",
  "O33vb7QuEM",
  "7yNClFD9yV",
  "EPQkiXNeA1",
  "whC0fNQizU",
  "J6wCv3B8eV",
  "2RGNVIuNLW",
  "n16SlDaJjt",
  "GyMMbTqmaG",
  "XkXpgxZDC7",
  "wCjERUJXYu",
  "WUR6oEy0hP",
  "yV015t0CYk",
  "jCrgn6dVt7",
  "abE6JUP1Aq",
  "wOLc539Brt",
  "ond9Wkax3g",
  "YBDLvwy4sZ",
  "bzn0Xlyeb3",
  "veWVF8L42F",
  "88wZQX7FLw",
  "r1hNPVCUSF",
  "lEMnjIxTkE",
  "8GcmAOBwvy",
  "vGoTSf9",
  "EZxoB3SikZ",
  "nXvpjXVN",
  "Th2tQq7OzP",
  "GZf2a87Hn9",
  "zG90lApgOS",
  "oqHQEuJixo",
  "3vcmAtdXop",
  "eu0yfkuku2",
  "8OkDSrZXpW",
  "OWODp9rhdf",
  "kaKCD2dlFt",
  "73fY0ixcbY",
  "o1kd3KTy8d",
  "zZKF7DZh40",
  "RH2bnImZax",
  "hX8bjc1XUb",
  "WLd3wamF4H",
  "UJUvZdeRND",
  "rJnnFVDbLw",
  "dlHpb7V1BY",
  "NDHwNc6LOm",
  "b",
  "3zieMDaSwQ",
  "HqWMdhaogx",
  "b2IfhXsT4h",
  "d5EeJe5Bsg",
  "AvVCQ5Yn80",
  "aecpnaHFim",
  "kyEOwqctPf",
  "ma9CEW1SPj",
  "fY2ZG8Vr9E",
  "r2vWpeXUcl",
  "ofw41j77qG",
  "FHRvjXzM2y",
  "jAuScIDrTv",
  "bC7V1CgLqZ",
  "uMuciVCbzi",
  "AjGKTQUpLZ",
  "bh2wa6TJlt",
  "GGUuQDI7ci",
  "kdUw8fdSDU",
  "xOYr06lWWz",
  "lALCyRAVLc",
  "JC4",
  "HV7XGR4F9s",
  "07jwWfNQV3",
  "7U9yMKd5XC",
  "gj1zeBk4uk",
  "wQ99aJUnq",
  "P0J1GHLk4s",
  "zG645HihJK",
  "xpQwcHOec1",
  "lfnqIVx8Yw",
  "Ys9VhbJJ",
  "LDGwDGNzCi",
  "lZtdpSyMD0",
  "S2QBNT45JF",
  "cWjhikn",
  "lMyoH57DI8",
  "VhmzxOPjh6",
  "iyDoVi0m4n",
  "12P5u",
  "3nsCbXUnwx",
  "BQbGEyMyRv",
  "Ldn1",
  "jUyX8Lq30G",
  "rlHtmxrfvH",
  "KBonf4HSHX",
  "Gz54m61RGD",
  "6OiuyQQM9P",
  "3V0tKCFLBk",
  "2AbpbkXqif",
  "bHvKHUtbQv",
  "qaNcLkPqei",
  "v5hb6Z7t32",
  "9ZzZtspwhC",
  "q7EFStLZF2",
  "aT11KGJZvP",
  "gk1CJ5Jwbg",
  "k34Qb9wNFN",
  "wTLkaF5QvW",
  "brZ0QWl\xFFiI",
  "0RMFBVgxsg",
  "zO8vrrOov2",
  "eGD4T0Dtl0",
  "mCQdLD\04p06",
  "SrOWOIBg9l",
  "416lu\0\0\0j2olZ",
  "KfftVnCXen",
  "NmAWR4OqAK",
  "KZAn6Gp\31XDn",
  "5Mazc7hWhE",
  "mkCi0QAv7V",
  "bGmB3qNjgK",
  "CjxezNsMWN" };

void htTest( void ){
  hasht* test = htNew();
  //  htPrint( test );
  htAdd( test, "lb\x11\x2a\x4\x0\30a", 8, "dogs\0\0\0dogs", 11 );
  //  htPrint( test );
  for( u32 i = 0; i < NUM_TEST_STRINGS; i += 2 ){
    if( !htFind( test, testStrings[ i ], slen( testStrings[ i ] ), 0 ) )
      htAdd( test, testStrings[ i ], slen( testStrings[ i ] ),
	     testStrings[ i + 1 ], slen( testStrings[ i + 1 ] ) );
    //htPrint( test );
    if( !( ( i + 1 ) % 3 ) ){
      u32 ind = ( ( i * 2981873 ) % ( i + 12455 ) ) % i;
      ind = ind ^ ( ind & 1 );
      //print( "Removing " ); printl( testStrings[ ind ] );
      if( htFind( test, testStrings[ ind ], slen( testStrings[ ind ] ), 0 ) ){
	htRemove( test, testStrings[ ind ], slen( testStrings[ ind ] ) );
	//htPrint( test );
      }else{
	//print( "...not found" );
      }
    }
  }
  htDestroy( test );
}
#endif
void htLoadDirectoryHelper( hasht* ht, fileNames* fns, u32 chop ){
  for( u64 i = 0; i < fns->numFiles; ++i ){
    new( name, char );
    strappend( &name, fns->dirName );
    strappend( &name, "\\" );
    strappend( &name, fns->files[ i ] );
    u32 size;
    char* f = loadFileOrDie( name, &size );
    htAddString( ht, name + chop, f, size );
    memfree( f );
    memfree( name );
  }
  for( u64 i = 0; i < fns->numDirs; ++i ){
    htLoadDirectoryHelper( ht, fns->subDirs[ i ], chop );
  }
}
hasht* htLoadDirectory( const char* dirname ){
  fileNames* fns = getFileNames( dirname );
  u32 chop = slen( fns->dirName ) + 1;
  hasht* ht = htNew();
  htLoadDirectoryHelper( ht, fns, chop );
  delFileNames( fns );
  return ht;
}


typedef struct{
  u32 hash;
  u64 keySize;
  u64 valueSize;
} serialBucket;
const char* htSerialize( const hasht* ht, u64* size ){
  u64 dataSize = 0;
  for( u64 i = 0; i < ht->size; ++i )
    dataSize += ( ht->data[ ht->used[ i ] ].keySize +
		  ht->data[ ht->used[ i ] ].valueSize );
  *size = sizeof( u64 ) + sizeof( serialBucket ) * ht->size + dataSize;
  u64* ret = mem( *size );
  serialBucket* sret = ((serialBucket*)( ret + 1 ));
  char* p = ((char*) ret) + sizeof( u64 ) +
    sizeof( serialBucket ) * ht->size;
  *ret = ht->size;
  for( u64 i = 0; i < *ret; ++i ){
    bucket* item = ht->data + ht->used[ i ];
    sret[ i ].hash = item->hash;
    sret[ i ].keySize = item->keySize;
    memcpy( p, item->key, item->keySize );
    p += item->keySize;
    sret[ i ].valueSize = item->valueSize;
    memcpy( p, item->value, item->valueSize );
    p += item->valueSize;
  }
    
  return (const char*)ret;
}
  
hasht* htDeserialize( const char* ser ){
  u64 size = *((u64*)ser);
  serialBucket* buckets = (serialBucket*)(((u64*)ser) + 1 );
  const char* data = ((char*) ser) + sizeof( u64 ) +
    sizeof( serialBucket ) * size;
  hasht* ret = htNew();
  for( u64 i = 0; i < size; ++i ){
    const char* key = data;
    data += buckets[ i ].keySize;
    const char* value = data;
    data += buckets[ i ].valueSize;
    htAddWithHash( ret, key, buckets[ i ].keySize, value,
		   buckets[ i ].valueSize, buckets[ i ].hash );
  }
  return ret;
}
