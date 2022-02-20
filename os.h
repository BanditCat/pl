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
void* copy( const void* d, u64 s );

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


// Convienience macros.
#define new( x, y ) y* x = (y*)( mem( sizeof( y ) ) )
#define newe( y ) ( (y*)( mem( sizeof( y ) ) ) )
#define newa( x, y, s ) y* x = (y*)( mem( sizeof( y ) * s ) )
#define newae( y, s ) ( (y*)( mem( sizeof( y )  * s ) ) )

// Allocate permanent memory. The memory that is freed at end.
void* memperm( u64 size );
void* memSimple( u64 size );
void memfreeSimple( void* p );


// Generates exceptions, memory is zerod.
#ifdef DEBUG
// Prints diagnostics if show is 1. Returns 1 iff memory is consistent,
// otherwise 0.
bool memCheck( bool show );
// Verifies addr is allocated, otherwise dies with a message about double frees.
void memCheckAddr( void* addr, const char* tag );
void* memDebug( u64 size, const char* tag );
void memfreeDebug( void* p, const char* tag );
#define mem( s ) memDebug( s, AT )
#define memfree( s ) memfreeDebug( s, AT ": bad free." )
#else
#define mem memSimple
#define memfree memfreeSimple
#endif


// Allocates the returned value, which should be freed.
// NUL terminated strings only.
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

// File functions.
typedef struct fileNames{
  char* dirName;
  u32 numDirs, numFiles;
  u64* fileSizes;
  char** files;
  struct fileNames** subDirs;
} fileNames;
// Dies if directory not found.
fileNames* getFileNames( const char* name );
void delFileNames( fileNames* dn );
// Loads the specified file name, or terminates on error. If size is not NULL,
// the integer it points to is set to the file size. Only supports files < 4gb.
// The returned array must be freed.
char* loadFileOrDie( const char* filename, u32* outSize );
void writeFileOrDie( const char* filename, const char* data, u64 dataSize );
const char* compressOrDie( const char* data, u64 dataSize, u64* outSize );
const char* uncompressOrDie( const char* data, u64 dataSize, u64* outSize );
