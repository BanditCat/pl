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
void* memset( void* dst, int chr, size_t size );
void* memmove( void* dst, const void* src, size_t num );
int memcmp( const void * ptr1, const void* ptr2, size_t num );

void print( const char* str );
void printl( const char* str );
#define endl() printl( "" )
void eprint( const char* str );
void eprintl( const char* str );
void message( const char* str );
void emessage( const char* str );
void end( int ecode );

// Generates exception if out of memory. Memory is zerod. Automatically freed at end.
void* mem( u64 size );

// Convienience macros.
#define new( x, y ) y* x = (y*)( mem( sizeof( y ) ) )
#define newe( y ) ( (y*)( mem( sizeof( y ) ) ) )
#define newa( x, y, s ) y* x = (y*)( mem( sizeof( y ) * s ) )
#define newae( y, s ) ( (y*)( mem( sizeof( y )  * s ) ) )

// Same, but doesnt increment the alloc count. This allocates memory that is freed at end.
void* memperm( u64 size );
void memfree( void* p );

#define die( x ) { emessage( AT "\n\n" x ); eprint( AT "\n" x "\n" ); end( 1 ); }

// Allocates the returned value, which should be freed. NUL terminated strings only.
u16* utf8to16( const char* str );
char* utf16to8( const u16* str );
char* utf16to8perm( const u16* str );

// Timing functions.
u64 tickFrequency( void );
u64 tickCount( void );

// This function returns a resource as a char array, terminating on error. The
// size is stored in size, if size is null, an error occurs. Resources must be
// of type "PLCUSTOM".
const char* loadBuiltin( const char* name, u32* size );

