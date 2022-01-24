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


#include "pl.h"
#include "os.h"
#include <windows.h>

int main( int argc, const char** argv );

int WINAPI __entry( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow ){
  (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
  return main( 0, NULL );
}

void print( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), str, l, NULL, NULL );
}

void eprint( const char* str ){
  u64 l = slen( str );
  WriteFile( GetStdHandle( STD_ERROR_HANDLE ), str, l, NULL, NULL );
}

void message( const char* message ){
  MessageBoxA( NULL, message, NAME, 0 );
}

void emessage( const char* message ){
  MessageBoxA( NULL, message, NAME, MB_ICONEXCLAMATION );
}

void end( int ecode ){
  ExitProcess( ecode );
}
