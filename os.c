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

states state;

int main( int argc, const char** argv );


void WINAPI __entry( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ){
  (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
  state.heap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 0, 0 );
  state.allocCount = 0;
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
  state.vk = plvkInit();
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
  plvkEnd( state.vk );
#ifdef DEBUG
  print( "Ending with " );
  printInt( state.allocCount );
  print( " unfreed allocs.\n" );
#endif
  FreeConsole();
  HeapDestroy( state.heap );
  ExitProcess( ecode );
}

// Generates exception if out of memory. Memory is zerod. Automatically freed at end.
void* mem( u64 size ){
  ++state.allocCount;
  return HeapAlloc( state.heap, HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY, size );
}
// Same, but doesnt increment the alloc count. This allocates memory that is freed at end.
void* memperm( u64 size ){
  return HeapAlloc( state.heap, HEAP_GENERATE_EXCEPTIONS + HEAP_ZERO_MEMORY, size );
}
void memfree( void* p ){
  --state.allocCount;
  HeapFree( state.heap, 0, p );
}

void* memcpy( void* dst, void const* src, size_t size ){
  for( u64 i = 0; i < size; ++i )
    ( (char*)dst )[ i ] = ( (const char*)src )[ i ];
  return dst;
}

// Timing functions.
u64 tickFrequency( void ){
  LARGE_INTEGER ret;
  if( !QueryPerformanceFrequency( &ret ) )
    die( "QueryPerformanceFrequency failed." );
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
