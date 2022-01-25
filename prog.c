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
// Program logic.                                                             //
////////////////////////////////////////////////////////////////////////////////

#include "pl.h"
#include "prog.h"
#include "os.h"
#include "util.h"


program* newProgram( u32 size ){
  new( ret, program );
  ret->stateSize = size;
  newa( ping, u32, size );
  ret->ping = ping;
  newa( pong, u32, size );
  ret->pong = pong;
  ret->state = ping;
  newa( args, u32, size );
  ret->args = args;
  ret->functionCount = 0;
  ret->functionsAllocd = 1;
  newa( functions, function, 1 );
  ret->functions = functions;
  newa( funcs, u32, size );
  ret->funcs = funcs;
  return ret;
}

void deleteProgram( program* p ){
  memfree( p->ping );
  memfree( p->pong );
  memfree( p->args );
  for( u32 i = 0; i < p->functionCount; ++i )
    memfree( p->functions[ i ].data );
  memfree( p->functions );
  memfree( p->funcs );
  memfree( p );
}



void addFunction( program* p, u8 a1s, u8 a2s, u32 (*f)( u32 ) ){
  // Ensure there is room for one more function.
  if( p->functionsAllocd == p->functionCount ){
    newa( nf, function, p->functionsAllocd * 2 );
    memcopy( nf, p->functions, function, p->functionCount );
    memfree( p->functions );
    p->functions = nf;
    p->functionsAllocd *= 2;
  }
  
  u32 fsize = 1 << ( a1s + a2s );
  newa( d, u32, fsize );
  p->functions[ p->functionCount ].data = d;
  p->functions[ p->functionCount ].arg1Size = a1s;
  p->functions[ p->functionCount ].arg2Size = a2s;
  for( u32 i = 0; i < fsize; ++i )
    *d++ = f( i );

  ++p->functionCount;
}

void printProgram( const program* p, bool full ){
  char* m = mem( 256 );
  intToString( m, p->stateSize, 256 );
  print( "Prog[ " ); print( m ); printl( " ]{" );
  printArray( 2, 8, p->stateSize, p->state );
  printl( "" );
  if( full ){
    printl( "  Funcs{" );
    printArray( 4, 8, p->stateSize, p->funcs );
    printl( "\n  }" );
    printl( "  Args{" );
    printArray( 4, 8, p->stateSize, p->args );
    printl( "\n  }" );
    for( u32 i = 0; i < p->functionCount; ++i ){
      u32 fsize = 1 << ( p->functions[ i ].arg1Size + p->functions[ i ].arg2Size );
      intToString( m, i, 256 );
      print( "  Func[ " ); print( m ); printl( " ]{" );
      printl( "" );
      printArray( 4, 8, fsize, p->functions[ i ].data );
      printl( "\n  }" );
    }
  }
  printl( "}" );
  memfree( m );
}
void tick( program* p ){
  u32* read = p->state;
  u32* write = ( read == p->ping ) ? p->pong : p->ping;
  for( u32 i = 0; i < p->stateSize; ++i ){
    const function* func = p->functions + p->funcs[ i ];
    u32 arg2shift = func->arg1Size;
    u32 arg1and = ( 1 << arg2shift ) - 1;
    u32 arg2and = ( 1 << func->arg2Size ) - 1;
    u32 arg = read[ i ] & arg1and;
    arg |= ( read[ p->args[ i ] ] & arg2and ) << arg2shift;
    write[ i ] = func->data[ arg ];
  }
  p->state = write;
}
void traceProgram( program* p, u32 stepCount ){
  printProgram( p, true );
  for( u32 i = 0; i < stepCount; ++i ){
    tick( p );
    printProgram( p, false );
  }
}

u32 inc( u32 x ){ return ( x + 1 ) % 64; }
u32 mul( u32 x ){ return ( ( x >> 6 ) * ( x & 63 ) ) % 64; }
u32 constant( u32 x ){ (void)x; return 42; }
void testPrograms( void ){
  program* p = newProgram( 16 );
  u32 args[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 11, 10, 13, 14, 15, 0 };
  memcopy( p->args, args, u32, 16 );
  for( u32 i = 0; i < 16; ++i ){
    p->funcs[ i ] = i % 3;
    p->state[ i ] = i;
  }
  
  addFunction( p, 6, 0, inc );
  addFunction( p, 0, 0, constant );
  addFunction( p, 6, 6, mul );
  traceProgram( p, 10 );
  deleteProgram( p );
}
