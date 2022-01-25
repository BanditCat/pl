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


program* newProgram( u64 size ){
  new( ret, program );
  ret->stateSize = size;
  newa( state, u32, size );
  newa( args, u32, size );
  ret->state = state;
  ret->args = args;
  ret->functionCount = 0;
  ret->functionsAllocd = 256;
  newa( functions, function, 256 );
  ret->functions = functions;
  newa( funcData, u32, 256 );
  ret->funcData = funcData;
  ret->funcDataAllocd = 256;
  return ret;
}

void deleteProgram( program* p ){
  freemem( p->state );
  freemem( p->args );
  freemem( p->functions );
  freemem( p->funcData );
  freemem( p );
}
