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
  newa( funcData, u32, 1 );
  ret->funcData = funcData;
  ret->funcDataAllocd = 1;
  ret->funcDataUsed = 0;
  newa( funcs, u32, size );
  ret->funcs = funcs;
  return ret;
}

void deleteProgram( program* p ){
  memfree( p->state );
  memfree( p->args );
  memfree( p->functions );
  memfree( p->funcData );
  memfree( p->funcs );
  memfree( p );
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
  u32 needed = p->funcDataUsed + fsize;
  // Ensure there is room for the funciton data.
  if( needed > p->funcDataAllocd ){
    u32 ns = p->funcDataAllocd;
    while( ns < needed )
      ns *= 2;
    newa( nfd, u32, ns );
    memcopy( p->funcData, nfd, p->funcDataUsed * sizeof( u32 ) );
    memfree( p->funcData );
    p->funcData = nfd;
    p->funcDataAllocd = ns;
  }
  u32* d = p->funcData + p->funcDataUsed;

  p->functions[ p->functionCount ].arg1Size = a1s;
  p->functions[ p->functionCount ].arg2Size = a2s;
  p->functions[ p->functionCount ].data = d;
  for( u32 i = 0; i < fsize; ++i )
    *d++ = f( i );

  ++p->functionCount;
}
