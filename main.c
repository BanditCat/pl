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
// Main entry point.                                                          //
////////////////////////////////////////////////////////////////////////////////

#include "pl.h"
#include "os.h"
#include "prog.h"
#include "util.h"
#include "gui.h"
#include "vk.h"

const char* clUsage =
  ////////////////////////////////////////
  ////////////////////////////////////////
  "Command line options:\n"
  "\n"
  "-window=<H>x<W>,<X>,<Y>    Set the size"
  " size of the window to H height at W \n"
  "                           width at the"
  " screen position specified by X and Y\n"
  "\n"
  "-gpu=<G>                   Use the gpu "
  "specified by G, which is an integer.\n"
  "\n"
  "-info                      List the gpu"
  "s and other information.\n"
  "\n"
  "-frameCount=<N>            Use n frames"
  "for rendering, aka pre rendering.\n"
  "\n"
  "-fps                       Print fps to "
  "stdout every second.\n"
  "\n"
  "-compressorOutput=name     The output fi"
  "lename for the compressor.\n"
  "\n"
  "-compressDir=dir           Compress the "
  "indicated directory.\n"
  "\n"
#ifdef DEBUG  
  "-debugLevel=<D>            Prevents val"
  "idation messages with severity less than\n"
  "                           D from being"
  " output. (default is 2).\n";
#else
;
#endif

// Default window position
#define defX 100
#define defY 100
#define minW 320
#define minH 240

int main( int argc, const char** argv ){
  u32 x = defX;
  u32 y = defY;
  u32 w = minW;
  u32 h = minH;
  // -1 means pick GPU based on score.
  int gpu = -1;
  u32 debugLevel = 2;

  // For compression.
  const char* compressorOutput = NULL;
  const char* compressDir = NULL;
  
  // Parse and execute command line.
  for( int i = 1; i < argc; ++i ){
    if( strStartsWith( "-window=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-window=" );
      const char* trackOpt = opt;
      w = parseInt( &opt );
      if( opt == trackOpt || *opt != 'x' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      h = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      x = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      y = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -window= command line option." );
      continue;
    }
    if( strStartsWith( "-gpu=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-gpu=" );
      const char* trackOpt = opt;
      gpu = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -gpu= command line option." );
      continue;
    }
    if( strStartsWith( "-frameCount=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-frameCount=" );
      const char* trackOpt = opt;
      *((u32*)&state.frameCount) = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -frameCount= command line option." );
      continue;
    }
    if( strStartsWith( "-compressorOutput=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-compressorOutput=" );
      if( !slen( opt ) )
	die( "Malformed -compressorOutput= command line option." );
      compressorOutput = opt;
      continue;
    }
    if( strStartsWith( "-compressDir=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-compressDir=" );
      if( !slen( opt ) )
	die( "Malformed -compressDir= command line option." );
      compressDir = opt;
      continue;
    }
#ifdef DEBUG    
    if( strStartsWith( "-debugLevel=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-debugLevel=" );
      const char* trackOpt = opt;
      debugLevel = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -debugLevel= command line option." );
      continue;
    }
#endif
    if( !strcomp( "-fps", argv[ i ] ) ){
      state.fps = 1;
      continue;
    }
    if( !strcomp( "-info", argv[ i ] ) ){
      plvkInit( gpu, debugLevel, TARGET, 10, 10, 320, 240 );
      plvkPrintInitInfo();
      plvkPrintGPUs();
      return 0;
    }
    if( !strcomp( argv[ i ], "-help" ) || !strcomp( argv[ i ], "--help" ) ||
	!strcomp( argv[ i ], "-?" ) || !strcomp( argv[ i ], "/?" ) ){
      printl( clUsage );
      return 0;
    }
    print( "Unrecognized command line option " );
    printl( argv[ i ] );
    printl( clUsage );
    return 1;
  }
  if( compressDir && !compressorOutput ){
    die( "Directory to compress given, but no output specified." );
  }else if( !compressDir && compressorOutput ){
    die( "Compression output file given, but no directory specified." );
  }else if( compressDir && compressorOutput ){
    hasht* ht = htLoadDirectory( compressDir );
    const char* htfs = htFindString( ht, "shaders\\mainFrag.spv", 0 );
    endl();endl();endl();endl();  printRaw( htfs, 100 );endl();endl();endl();
    u64 ssize;
    const char* sd = htSerialize( ht, &ssize );
    u64 csize;
    compressOrDie( sd, ssize, &csize );
    memfree( (void*)sd );
    print( "osize: " ); printInt( ssize ); endl();
    print( "csize: " ); printInt( csize ); endl();
    print( "diff: " ); printInt( ssize - csize ); endl();
 
    print( "Ratio: " ); printFloat( (f64)( csize ) / (f64)ssize ); endl();
    htDestroy( ht );
    return 0;
  }

  // Initialize vulkan.
#ifdef DEBUG
  printl( "Initializing vulkan..." );
#endif
  plvkStatep vk = plvkInit( gpu, debugLevel, TARGET, x, y, w, h );
  // Main loop.
  plvkShow( vk );
  while( plvkeventLoop( vk ) )
    draw();
  // Run tests.
#ifdef DEBUG
  testPrograms();
  htTest();
#endif
  return 0;
}

