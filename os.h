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


void* memcpy( void* dst, void const* src, size_t size );

void print( const char* str );
void printl( const char* str );
void eprint( const char* str );
void eprintl( const char* str );
void message( const char* str );
void emessage( const char* str );
void end( int ecode );

// Generates exception if out of memory. Memory is zerod. Automatically freed at end.
void* mem( u64 size );

// Convienience macros.
#define new( x, y ) y* x = (y*)( mem( sizeof( y ) ) )
#define newa( x, y, s ) y* x = (y*)( mem( sizeof( y ) * s ) )

// Same, but doesnt increment the alloc count. This allocates memory that is freed at end.
void* memperm( u64 size );
void memfree( void* p );

#define die( x ) emessage( AT "\n\n" x ); eprint( AT "\n" x "\n" ); end( 1 )
