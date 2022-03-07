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
// OS logic.                                                                  //
////////////////////////////////////////////////////////////////////////////////


#define UNICODE
#include <windows.h>
#include <compressapi.h>

#include "pl.h"
#include "os.h"
#include "util.h"
#include "vk.h"

#define MEMCONST1 0x12FEEDFACEC0FFEE
#define MEMCONST2 0xDEADBEEFDEADBEEF
#define MEMCONST3 '~'
#ifdef DEBUG
typedef struct memtag{
  const char* tag;
  u64 index;
  u64 check;
  u64 size;
} memtag;
#endif

states state;

int main( int argc, const char** argv );

// lol
u32 _fltused = 1;

void WINAPI __entry( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ){
  (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
  state.heap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 0, 0 );
  state.ended = 0;
  
#ifdef DEBUG
  state.memallocCount = 0;
  state.memnextFree = 0;
  state.memindices = memperm( sizeof( u64 ) * 256 );
  state.memallocd = memperm( sizeof( char* ) * 256 );
  state.memindicesAllocated = 256;
  for( u64 i = 0; i < state.memindicesAllocated; ++i )
    state.memindices[ i ] = i + 1;
#endif
  
  AttachConsole( ATTACH_PARENT_PROCESS );
  SetConsoleOutputCP( CP_UTF8 );

  //Convert commandline to utf-8 and store in state.
  u32 size = 0;
  u16** cl = CommandLineToArgvW( GetCommandLine(), &state.argc );
  if( cl != NULL ){
    state.argv = memperm( sizeof( char* ) * state.argc );
    for( int i = 0; i < state.argc; ++i ){
      if( cl[ i ] == NULL )
	die( "NULL command line argument!" );
      if( !cl[ i ][ 0 ] )
	die( "Empty command line argument!" );
      state.argv[ i ] = utf16to8perm( cl[ i ] );
    }
    LocalFree( cl );
  } else
    state.argc = 0;
  if( !state.argc ){
    u32 ts = slen( TARGET ) + 1;
    size = sizeof( char* ) + ts;
    state.argv = memperm( sizeof( char* ) * 1 );
    *state.argv = memperm( slen( TARGET ) + 1 );
    strcopy( ((char*)*state.argv), TARGET);
    state.argc = 1;
  }
  state.vk = NULL;
#ifdef DEBUG
  state.fps = 1;
#else
  state.fps = 0;
#endif
  // Get compressed resources.
  {
    u32 cresSize;
    u64 resSize;
    const char* cres = loadBuiltin( "cres", &cresSize );
    const char* res = uncompressOrDie( cres, cresSize, &resSize );
    state.compressedResources = htDeserialize( res );
    memfree( (void*)res );
  }
  end( main( state.argc, state.argv ) );
}

void print( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), str, l, NULL, NULL );
}
void printl( const char* str ){
  print( str );
  print( "\n" );
}

void eprint( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_ERROR_HANDLE ), str, l, NULL, NULL );
}
void eprintl( const char* str ){
  eprint( str );
  eprint( "\n" );
}

void message( const char* message ){
  wchar_t* c = (wchar_t*)utf8to16( message );
  wchar_t* t = (wchar_t*)utf8to16( NAME );
  MessageBox( NULL, c, t, 0 );
  memfree( c );
  memfree( t );
}

void emessage( const char* message ){
  wchar_t* c = (wchar_t*)utf8to16( message );
  wchar_t* t = (wchar_t*)utf8to16( NAME );
  MessageBox( NULL, c, t, MB_ICONEXCLAMATION );
  memfree( c );
  memfree( t );
}

void end( int ecode ){
  if( !state.ended ){
    state.ended = 1;
    if( state.compressedResources )
      htDestroy( state.compressedResources );
    if( state.vk )
      plvkEnd( state.vk );
#ifdef DEBUG
    memCheck( 1 );
    eprintl( "Exiting from end()." );
#endif
    FreeConsole();
    HeapDestroy( state.heap );
  }
  ExitProcess( ecode );
}

#ifdef DEBUG
bool memCheck( bool show ){
  u64 ac = 0;
  if( show ){
    eprintInt( state.memallocCount );
    eprintl( " unfreed allocs." );
  }
  for( u64 i = 0; i < state.memindicesAllocated; ++i ){
    if( state.memallocd[ i ] ){
      memtag* mt = (memtag*)state.memallocd[ i ];
      const u8* mtc = (const u8*)mt;
      ++ac;
      if( show ){
	eprint( "Index " );
	eprintInt( i );
	eprint( " size " );
	eprintInt( mt->size - sizeof( memtag ) - 1 ); endl();
	eprint( mt->tag );
	eprint( ": " );
      }
      if( MEMCONST1 != ( mt->check ^ mt->index ) ){
	if( show ) eprintl( "Memory begin constant check failed." );
	return 0;
      }else if( MEMCONST3 != mtc[ mt->size - 1 ] ){
	if( show ) eprintl( "Memory end constant check failed." );
	return 0;
      }else if( mt->index != i ){
	if( show ) eprintl( "Memory index check failed." );
	return 0;
      }else{
	if( show ) eprintl( "good" );
      }
    }
  }
  if( ac != state.memallocCount ){
    if( show ) eprintl( "Memory count check failed." );
    return 0;
  }
  return 1;      
}
void memCheckAddr( void* addrarg, const char* tag, bool show ){
  void* addr = ((char*)addrarg) - sizeof( memtag );
  memtag* mt = addr;
  if( show ){
    print( "\nMemCheck on address " );
    printIntInBase( (u64)addrarg, 16 );
    print( " {\n  tag:   " );
    print( mt->tag );
    print( "\n  check: " );
    printIntInBase( mt->check, 16 );
    print( "\n  size:  " );
    printInt( mt->size );
    print( "\n  raw:   " );
    printRaw( (const char*)( mt + 1 ), mt->size - sizeof( memtag ) );
    print( "\n}\n" );
  }
  for( u64 i = 0; i < state.memindicesAllocated; ++i ){
    if( state.memallocd[ i ] == addr ){
      const u8* mtc = (const u8*)mt;
      if( MEMCONST1 != ( mt->check ^ mt->index ) ){
	die( tag );
      }else if( MEMCONST3 != mtc[ mt->size - 1 ] ){
	die( tag );
      }else if( mt->index != i ){
	die( tag );
      }
      return;
    }
  }
  eprint( tag );
  die( "Address not found, possible double free." );      
}
void* memDebug( u64 size, const char* tag ){
  memc;
  // Make room if necessary.
  if( state.memnextFree >= state.memindicesAllocated ){
    u64 tc = state.memindicesAllocated;
    state.memindicesAllocated *= 2;
    u64* t = memperm( sizeof( u64 ) * state.memindicesAllocated );
    for( u64 i = 0; i < state.memindicesAllocated; ++i )
      if( i < tc )
	t[ i ] = state.memindices[ i ];
      else
	t[ i ] = i + 1;
    memfreeSimple( state.memindices );
    state.memindices = t;
    char** ta = memperm( sizeof( char* ) * state.memindicesAllocated );
    memcopy( ta, state.memallocd, char*, tc );
    memfreeSimple( state.memallocd );
    state.memallocd = ta;
  }    
    
  u8* mem = HeapAlloc( state.heap,
		       HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY,
		       size + sizeof( memtag ) + 1 );
  mem[ size + sizeof( memtag ) ] = MEMCONST3;
  memtag* mt = (memtag*)mem;
  mt->tag = tag;
  mt->index = state.memnextFree;
  state.memallocd[ mt->index ] = (char*)mt;
  state.memnextFree = state.memindices[ state.memnextFree ];
  mt->check = MEMCONST1 ^ mt->index;
  mt->size = size + sizeof( memtag ) + 1;
  
  ++state.memallocCount;
  memc;
  return mem + sizeof( memtag );
}
void memfreeDebug( void* vp, const char* tag ){
  memc;
  memCheckAddr( vp, tag, 0 );
  char* p = ((char*)vp) - sizeof( memtag );
  memtag* mt = (memtag*)p;
  if( MEMCONST1 != ( mt->check ^ mt->index ) )
    die( tag );
  mt->check = MEMCONST2;
  u64 t = state.memnextFree;
  state.memallocd[ mt->index ] = NULL;
  state.memnextFree = mt->index;
  state.memindices[ mt->index ] = t;
  
  mt->index = 0xFFFFFFFFFFFFFFFF;
  
  
  --state.memallocCount;
  HeapFree( state.heap, 0, p );
  memc;
}

#endif
void* memSimple( u64 size ){
  return HeapAlloc( state.heap, HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY,
		    size );
}
void memfreeSimple( void* p ){
  HeapFree( state.heap, 0, p );
}

void* memperm( u64 size ){
  return HeapAlloc( state.heap, HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY,
		    size );
}



void* memcpy( void* dst, void const* src, size_t size ){
  for( u64 i = 0; i < size; ++i )
    ( (char*)dst )[ i ] = ( (const char*)src )[ i ];
  return dst;
}
void* copy( const void* d, u64 s ){
  newa( ret, char, s );
  memcpy( ret, d, s );
  return ret;
}

void* memset( void* dst, int chr, size_t size ){
  for( u64 i = 0; i < size; ++i )
    ( (char*)dst )[ i ] = (char)chr;
  return dst;
}
void* memmove( void* dst, const void* src, size_t num ){
  if( src < dst ){
    for( s64 i = num - 1; i >= 0; --i )
      ( (char*)dst )[ i ] = ( (const char*)src )[ i ];
  }else{
    for( u64 i = 0; i < num; ++i )
      ( (char*)dst )[ i ] = ( (const char*)src )[ i ];
  }
  return dst;
}
int memcmp( const void* ptr1, const void *ptr2, size_t num ){
  const char* xp = ptr1;
  const char* yp = ptr2;
  u64 c = 0;
  while( c < num ){
    if( *xp < *yp )
      return -1;
    if( *xp > *yp )
      return 1;
    ++xp;
    ++yp;
    ++c;
  }
  return 0;
}

// Timing functions.
u64 tickFrequency( void ){
  static LARGE_INTEGER ret;
  ret.QuadPart = 0;
  if( !ret.QuadPart ){
    if( !QueryPerformanceFrequency( &ret ) )
      die( "QueryPerformanceFrequency failed." );
  }
  return ret.QuadPart;
}
u64 tickCount( void ){
  LARGE_INTEGER ret;
  if( !QueryPerformanceCounter( &ret ) )
    die( "QueryPerformanceCounter failed." );
  return ret.QuadPart;
}

u16* utf8to16( const char* str ){
  int size = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
  newa( ws, u16, size );
  MultiByteToWideChar( CP_UTF8, 0, str, -1, ws, size );
  return ws;
}
char* utf16to8( const u16* str ){
  int size = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
  newa( s, char, size );
  WideCharToMultiByte( CP_UTF8, 0, str, -1, s, size, NULL, NULL );
  return s;
}
char* utf16to8perm( const u16* str ){
  int size = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
  char* s = memperm( size );
  WideCharToMultiByte( CP_UTF8, 0, str, -1, s, size, NULL, NULL );
  return s;
}
const char* loadBuiltin( const char* name, u32* size ){
  HRSRC r = FindResourceA( NULL, name, "PLCUSTOM" );
  if( NULL == r )
    die( "Failed to find resource." );
  HGLOBAL rd = LoadResource( NULL, r );
  if( NULL == rd )
    die( "Failed to load resource." );
  const char* res = LockResource( rd );
  if( NULL == size )
    die( "Null pointer passed to loadBuiltin." );
  *size = SizeofResource( NULL, r );
  if( !(*size) )
    die( "Zero size resource." );
  if( NULL == res )
    die( "Failed to lock resource." );
  return res;
}
#define MAX_PATH_LENGTH 32000
fileNames* getFileNames( const char* nameArg ){
  u32 cdlen = GetCurrentDirectoryW( 0, NULL ) + 2;
  newa( cd16, u16, cdlen );
  GetCurrentDirectoryW( cdlen, cd16 );
  char* cd = utf16to8( cd16 );
  memfree( cd16 );
  u32 len = slen( nameArg ) + 5;
  new( rname, char );
  strappend( &rname, "\\\\?\\" );
  strappend( &rname, cd );
  memfree( cd );
  if( slen( nameArg ) )
    strappend( &rname, "\\" );
  strappend( &rname, nameArg );
  newa( name, char, slen( rname ) + 5 );
  strcopy( name, rname );
  strappend( &name, "\\*" );
  len = 0;
  u16* nameu = utf8to16( name );
  memfree( name );
  while( nameu[ len ] )
    len++;
  if( len > MAX_PATH_LENGTH )
    die( "Path too long in getFiles." );
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( nameu, &fd );
  if( INVALID_HANDLE_VALUE == h )
    die( "Directory listing failed." );
  u32 fcount = 0;
  u32 dcount = 0;
  do{
    char* fname = utf16to8( fd.cFileName );
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
	&& strcomp( fname, "." ) && strcomp( fname, ".." ) )
      ++dcount;
    else if( !( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
      ++fcount;
    memfree( fname );
  }while( FindNextFileW( h, &fd ) );
  new( ret, fileNames );
  ret->numDirs = dcount;
  ret->subDirs = newae( fileNames*, dcount );
  ret->numFiles = fcount;
  ret->files = newae( char*, fcount );
  ret->fileSizes = newae( u64, fcount );
  ret->dirName = rname + 4;
  fcount = 0;
  dcount = 0;
  h = FindFirstFileW( nameu, &fd ); 
  do{
    char* fname = utf16to8( fd.cFileName );
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
	&& strcomp( fname, "." ) && strcomp( fname, ".." ) ){
      if( dcount >= ret->numDirs )
	die( "Unstable filesystem." );
      newa( sd, char, slen( nameArg ) + slen( fname ) + 5 );
      if( slen( nameArg ) ){
	strappend( &sd, nameArg );
	strappend( &sd, "\\" );
      }
      strappend( &sd, fname );
      ret->subDirs[ dcount ] = getFileNames( sd );
      memfree( sd );
      memfree( fname );
      ++dcount;
    }else if( !( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ){
      if( fcount >= ret->numFiles )
	die( "Unstable filesystem." );
      ret->files[ fcount ] = fname;
      ret->fileSizes[ fcount ] = ( ((u64)fd.nFileSizeHigh) << 32 ) |
	((u64)fd.nFileSizeLow);
      ++fcount;
    }else
      memfree( fname );
  }while( FindNextFileW( h, &fd ) );

  memfree( nameu );
  return ret;
}
void delFileNames( fileNames* dn ){
  /* print( "Directory: " ); print( dn->dirName ); printl( "[" ); */
  /* print( "  number of files: " ); printInt( dn->numFiles ); endl(); */
  /* print( "  number of dirs: " ); printInt( dn->numDirs ); endl(); */
  /* printl( "  dirs[" ); */
  /* for( u64 i = 0; i < dn->numDirs; ++i ){ */
  /*   print( "    " ); printl( dn->subDirs[ i ]->dirName ); */
  /* } */
  /* printl( "  ]" ); */
  /* printl( "  files[" ); */
  /* for( u64 i = 0; i < dn->numFiles; ++i ){ */
  /*   print( "    " ); print( dn->files[ i ] ); */
  /*   print( ", size " ); printInt( dn->fileSizes[ i ] ); endl(); */
  /* } */
  /* printl( "  ]\n]" ); */

  for( u64 i = 0; i < dn->numDirs; ++i )
    delFileNames( dn->subDirs[ i ] );
  for( u64 i = 0; i < dn->numFiles; ++i )
    memfree( dn->files[ i ] );
  memfree( dn->subDirs );
  memfree( dn->files );
  memfree( dn->fileSizes );
  memfree( dn->dirName - 4 );
  memfree( dn );
}
char* loadFileOrDie( const char* filename, u32* size ){
  HANDLE h;

  u16* wname = utf8to16( filename );
  h = CreateFileW( wname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		   FILE_ATTRIBUTE_NORMAL, NULL );
  memfree( wname );
  if( INVALID_HANDLE_VALUE == h )
    die( "Failed to open file." );
  DWORD hfsize;
  u32 fsize = GetFileSize( h, &hfsize );
  if( INVALID_FILE_SIZE == fsize ) 
    die( "Failed to get file size." );
  if( hfsize ) 
    die( "File too large." );
  newa( buf, char, fsize + 4 );
  DWORD read;
  if( !ReadFile( h, buf, fsize + 8, &read, NULL ) )
    die( "Failed to read file." );
  if( read != fsize )
    die( "Failed to read file." );
  if( size )
    *size = read;
  buf[ read ] = 0;
  buf[ read + 1 ] = 0;
  CloseHandle( h );
  return buf;
}
void writeFileOrDie( const char* filename, const char* data, u64 dataSize ){
  HANDLE h;

  u16* wname = utf8to16( filename );
  h = CreateFileW( wname, GENERIC_WRITE, 0, NULL, CREATE_NEW,
		   FILE_ATTRIBUTE_NORMAL, NULL );
  memfree( wname );
  if( INVALID_HANDLE_VALUE == h )
    die( "Failed to create file for writing. Does it already exist?" );
  DWORD written;
  if( !WriteFile( h, data, dataSize, &written, NULL ) )
    die( "Failed to write file." );
  if( written != dataSize )
    die( "Failed to completly write file." );
  CloseHandle( h );
}
static const u32 ctypes[ 4 ] = { COMPRESS_ALGORITHM_MSZIP,
  COMPRESS_ALGORITHM_XPRESS, COMPRESS_ALGORITHM_XPRESS_HUFF,
  COMPRESS_ALGORITHM_LZMS };
const char* compressOrDie( const char* data, u64 dataSize, u64* outSize ){
  COMPRESSOR_HANDLE comp;
  u32 numCtypes = sizeof( ctypes ) / sizeof( ctypes[ 0 ] );
  size_t smallest = ((size_t)-1);
  const char* ret = NULL;
  for( u32 i = 0; i < numCtypes; ++i ){
    if( !CreateCompressor( ctypes[ i ], NULL, &comp ) )
      die( "Failed to create compressor." );
    newa( buf, char, dataSize + 258 );
    ++buf;
    size_t thisSize;
    if( !Compress( comp, data, dataSize, buf, dataSize + 257, &thisSize ) )
      die( "Failed to decompress data." );
    ++thisSize;
    --buf;
    if( thisSize < smallest ){
      *buf = (char)i;
      ret = buf;
      smallest = thisSize;
    }else
      memfree( buf );
    CloseCompressor( comp );
  }
  *outSize = smallest;
  return ret;
}
const char* uncompressOrDie( const char* data, u64 dataSize, u64* outSize ){
  DECOMPRESSOR_HANDLE decomp;
  if( !CreateDecompressor( ctypes[ (u32)(*data) ], NULL, &decomp ) )
    die( "Failed to create compressor." );
  Decompress( decomp, data + 1, dataSize - 1, "", 0, outSize );
  newa( buf, char, *outSize + 1 );
  u64 size = *outSize;
  if( !Decompress( decomp, data + 1, dataSize - 1, buf, size, outSize ) ){
    print( "\n\n\n\n" );printInt( GetLastError() );
    die( "Failed to decompress data." );
  }
  CloseCompressor( decomp );
  return buf;
}
void thread( unsigned long (*func)( void* ), void* p ){
  CreateThread( NULL, 0, func, p, 0, NULL );
}
