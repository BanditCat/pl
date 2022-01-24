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


#include "pl.h"
#include "os.h"
#include "util.h"
#include <windows.h>

states state;

int main( int argc, const char** argv );

int WINAPI __entry( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ){
  (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
  state.heap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 0, 0 );
  state.allocCount = 0;
  return main( 0, NULL );
}

void print( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), str, l, NULL, NULL );
}

void eprint( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_ERROR_HANDLE ), str, l, NULL, NULL );
}

void message( const char* message ){
  MessageBoxA( NULL, message, NAME, 0 );
}

void emessage( const char* message ){
  MessageBoxA( NULL, message, NAME, MB_ICONEXCLAMATION );
}

void end( int ecode ){
#ifdef DEBUG
  char* m = memperm( 256 );
  tostring( m, 4534546254366, 256 );
  print( "Ending with " );
  print( m );
  print( " unfreed allocs.\n" );
#endif
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
void freemem( void* p ){
  --state.allocCount;
  HeapFree( state.heap, 0, p );
}
