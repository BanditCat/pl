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

const char* clUsage =
  ////////////////////////////////////////////////////////////////////////////////
  "Command line options:\n"
  "\n"
  "-window=<H>x<W>,<X>,<Y>    Set the size size of the window to H height at W \n"
  "                           width at the screen position specified by X and Y\n"
  "\n"
  "-gpu=<G>                   Use the gpu specified by G, which is an integer.\n"
  "                           The output of this program contains the gpu numbers.\n";

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

  // Parse and execute command line.
  for( int i = 1; i < argc; ++i ){
    if( strStartsWith( "-window=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-window=" );
      const char* trackOpt = opt;
      w = parseInt( &opt );
      if( opt == trackOpt || *opt != 'x' )
	die( "Malformed -window command line option." );
      ++opt;
      trackOpt = opt;
      h = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window command line option." );
      ++opt;
      trackOpt = opt;
      x = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window command line option." );
      ++opt;
      trackOpt = opt;
      y = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -window command line option." );
      continue;
    }
    if( strStartsWith( "-gpu=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-gpu=" );
      const char* trackOpt = opt;
      gpu = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -gpu command line option." );
      continue;
    }
    if( strcomp( argv[ i ], "-help" ) && strcomp( argv[ i ], "--help" ) && strcomp( argv[ i ], "-?" ) ){
      print( "Unrecognized command line option " );
      printl( argv[ i ] );
    }
    printl( clUsage );
    return 0;
  }

  // Initialize vulkan
  state.vk = plvkInit( gpu );

  // Main loop.
  guiInfo* gui = wsetup( TARGET, x, y, w, h );
  while( weventLoop( gui ) )
    ;
  wend( gui );

  testPrograms();
  return 0;
}

