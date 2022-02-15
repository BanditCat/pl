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

#include "pl.h"
#include "os.h"
#include "util.h"
#include "vk.h"

#define MEMCONST1 0x12FEEDFACEC0FFEE
#define MEMCONST2 0xDEADBEEFDEADBEEF
#ifdef DEBUG
typedef struct{
  const char* tag;
  u64 index;
  u64 check;
} memtag;
#endif

states state;

int main( int argc, const char** argv );

// lol
u32 _fltused = 1;

void WINAPI __entry( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ){
  (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
  state.heap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 0, 0 );

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
  *((u32*)&state.frameCount) = 2;
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
  if( state.vk )
    plvkEnd( state.vk );
#ifdef DEBUG
  if( !state.memallocCount ){
    print( "Memory consistency test indicates success." );
  }else{
      
    print( "Ending with " );
    printInt( state.memallocCount );
    print( " unfreed allocs.\n" );
    for( u64 i = 0; i < state.memindicesAllocated; i++ ){
      if( state.memallocd[ i ] ){
	memtag* mt = (memtag*)state.memallocd[ i ];
	printInt( i );
	print( ": " );
	if( MEMCONST1 == ( mt->check ^ mt->index ) ){
	  print( "good " ); print( mt->tag );
	}else
	  print( "bad" );
	endl();
      }
    }
  }
#endif
  FreeConsole();
  HeapDestroy( state.heap );
  ExitProcess( ecode );
}

#ifdef DEBUG
void* memDebug( u64 size, const char* tag ){
  // Make room if necessary.
  if( state.memnextFree >= state.memindicesAllocated ){
    u64 tc = state.memindicesAllocated;
    state.memindicesAllocated *= 2;
    u64* t = memperm( sizeof( u64 ) * state.memindicesAllocated );
    for( u64 i = 0; i < state.memindicesAllocated; ++ i )
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
    
  char* mem = HeapAlloc( state.heap,
			 HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY,
			 size + sizeof( memtag ) );

  memtag* mt = (memtag*)mem;
  mt->tag = tag;
  mt->index = state.memnextFree;
  state.memallocd[ mt->index ] = (char*)mt;
  state.memnextFree = state.memindices[ state.memnextFree ];
  mt->check = MEMCONST1 ^ mt->index;
  
  ++state.memallocCount;
  return mem + sizeof( memtag );
}
void memfreeDebug( void* vp, const char* tag ){
  char* p = ((char*)vp) - sizeof( memtag );
  memtag* mt = (memtag*)p;
  mt->check ^= mt->index;
  if( mt->check != MEMCONST1 )
    die( tag );
  u64 t = state.memnextFree;
  state.memallocd[ mt->index ] = NULL;
  state.memnextFree = mt->index;
  state.memindices[ mt->index ] = t;
  
  mt->check = MEMCONST2 ^ mt->index;
  mt->index = 0xFFFFFFFFFFFFFFFF;
  
  
  --state.memallocCount;
  HeapFree( state.heap, 0, p );
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
