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
// win32 GUI.                                                                 //
////////////////////////////////////////////////////////////////////////////////

#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "pl.h"
#include "gui.h"
#include "os.h"
#include "util.h"

#define className ( L"plClassName" )

LONG WINAPI eventLoop( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
  
typedef struct{
  WNDCLASS wc;
  u16* title;
  HINSTANCE instance;
  HWND hWnd;
  HDC hDC;
  bool quit;
} guiState;


// Returned pointer must be deallocated with wend.
guiInfo* wsetup( char* title, int x, int y, int width, int height ){
  (void)title;(void)x;(void)y;(void)width;(void)height;
  new( ret, guiInfo );
  new( gui, guiState );
  ret->gui = gui;
  ret->width = width;
  ret->height = height;
  ret->x = x;
  ret->y = y;
  ret->fullScreen = 0;
  ret->screenWidth = GetSystemMetrics( SM_CXSCREEN );
  ret->screenHeight = GetSystemMetrics( SM_CYSCREEN );

  gui->instance = GetModuleHandle( NULL );
  gui->wc.style = CS_OWNDC;
  gui->wc.lpfnWndProc = (WNDPROC)eventLoop;
  gui->wc.cbClsExtra = 0;
  gui->wc.cbWndExtra = sizeof( guiInfo* ) * 5;
  
  gui->wc.hInstance = gui->instance;
  gui->wc.hIcon = LoadIcon( gui->instance, MAKEINTRESOURCE( 101 ) );
  gui->wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  gui->wc.hbrBackground = NULL;
  gui->wc.lpszMenuName = NULL;
  gui->wc.lpszClassName = className;

  RegisterClassW( &gui->wc );
  
  gui->title = utf8to16( title );
  gui->hWnd = CreateWindowExW( WS_EX_WINDOWEDGE, className, gui->title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			       x, y, width, height, NULL, NULL, gui->instance, NULL );
  SetWindowLongPtr( gui->hWnd, 0, (LONG_PTR)ret );
  gui->hDC = GetDC( gui->hWnd );
  ShowWindow( gui->hWnd, SW_SHOW );

  return ret;
}

void wend( guiInfo* p ){
  guiState* gui = p->gui;
  ReleaseDC( gui->hWnd, gui->hDC );
  DestroyWindow( gui->hWnd );
  DestroyIcon( gui->wc.hIcon );
  if( gui->title )
    memfree( gui->title );
  if( p->gui )
    memfree( p->gui );
  memfree( p );
}
bool weventLoop( guiInfo* p ){
  guiState* gui = p->gui;
  MSG msg;

  while( PeekMessage( &msg, gui->hWnd, 0, 0, PM_REMOVE )){
    TranslateMessage( &msg );
    DispatchMessageW( &msg );
  }
  return !gui->quit;
}


LONG WINAPI eventLoop( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ){
  guiInfo* p = (guiInfo*)GetWindowLongPtr( hWnd, 0 );
  if( !p )
    return (LONG)DefWindowProc( hWnd, uMsg, wParam, lParam );
  guiState* gui = p->gui;
  RECT r;

  switch(uMsg){
  case WM_SIZE:
    p->clientWidth = p->width = LOWORD( lParam );
    p->clientHeight = p->height = HIWORD( lParam );
    r.left = p->x;
    r.top = p->y;
    r.right = r.left + p->width;
    r.bottom = r.top + p->height;
    AdjustWindowRect( &r, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, FALSE );
    p->width = r.right - r.left;
    p->height = r.bottom - r.top;
    return 0;

  case WM_MOVE:
    p->x = LOWORD( lParam );
    p->y = HIWORD( lParam );
    r.left = p->x;
    r.top = p->y;
    r.right = r.left + p->width;
    r.bottom = r.top + p->height;
    AdjustWindowRect( &r, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, FALSE );
    p->x = r.left;
    p->y = r.top;
    return 0;


  case WM_CLOSE:
    gui->quit = 1;
    return 0;
  }
  return (LONG)DefWindowProc( hWnd, uMsg, wParam, lParam );
}
