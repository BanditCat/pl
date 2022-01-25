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


typedef struct{
  u32* data;
  u8 arg1Size;
  u8 arg2Size;
} function;

typedef struct{
  u32 stateSize;
  u32* ping;
  u32* pong;
  u32* state;
  u32* args;
  u32* funcs;
  
  u32 functionCount;
  function* functions;
  u32 functionsAllocd;

  
} program;


program* newProgram( u32 size );
void deleteProgram( program* p );
void addFunction( program* p, u8 a1s, u8 a2s, u32 (*f)( u32 ) );
void printProgram( const program* p, bool full );
void traceProgram( program* p, u32 stepCount );
void testPrograms( void );
