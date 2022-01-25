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
  newa( state, u32, size );
  newa( args, u32, size );
  ret->state = state;
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
  memfree( p->state );
  memfree( p->args );
  for( u32 i = 0; i < p->functionCount; ++i )
    memfree( p->functions[ i ].data );
  memfree( p->functions );
  memfree( p->funcs );
  memfree( p );
}

const function* newFunction( u8 a1s, u8 a2s, u32 (*f)( u32 ) ){
  u32 fsize = 1 << ( a1s + a2s );
  new( ret, function );
  newa( d, u32, fsize );
  ret->data = d;
  ret->arg1Size = a1s;
  ret->arg2Size = a2s;
  for( u32 i = 0; i < fsize; ++i )
    *d++ = f( i );
  return ret;
}  


void addFunction( program* p, u8 a1s, u8 a2s, u32 (*f)( u32 ) ){
  // Ensure there is room for one more function.
  if( p->functionsAllocd == p->functionCount ){
    newa( nf, function, p->functionsAllocd * 2 );
    memcopy( p->functions, nf, sizeof( function ) * p->functionCount );
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

void printProgram( const program* p ){
  char* m = mem( 256 );
  tostring( m, p->stateSize, 256 );
  print( "Prog[ " ); print( m ); printl( " ]{" );
  printArray( 2, 8, p->stateSize, p->state );
  printl( "}" );
  memfree( m );
}

void testPrograms( void ){
  program* p = newProgram( 64 );
  printProgram( p );
  deleteProgram( p );
}
