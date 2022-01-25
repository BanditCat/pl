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
  u64 stateSize;
  u32* state;
  u32* args;
  
  u64 functionCount;
  function* functions;
  u64 functionsAllocd;
  u32* funcData;
  u64 funcDataAllocd;
  
} program;


program* newProgram( u64 size );
void deleteProgram( program* p );
