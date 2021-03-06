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
#include <hidsdi.h>

#include "pl.h"
#include "vk.h"
#include "gui.h"
#include "util.h"
#include "os.h"
#include "input.h"

#define className ( L"plClassName" )


typedef struct guiState{
  WNDCLASS wc;
  u16* title;
  HDC hDC;
  bool quit;
  HINSTANCE instance;
} guiState;


LONG WINAPI eventLoop( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
  

// Returned pointer must be deallocated with wend.
guiInfo* wsetup( const char* title, int x, int y, int width, int height ){
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
  ret->handle = CreateWindowExW( WS_EX_WINDOWEDGE, className, gui->title,
				 WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS |
				 WS_CLIPCHILDREN,
				 x, y, width, height, NULL, NULL,
				 gui->instance, NULL );
  //remove all window styles, check MSDN for details
  //  SetWindowLong( ret->handle, GWL_STYLE, 0 );
  SetWindowLongPtr( ret->handle, 0, (LONG_PTR)ret );
  gui->hDC = GetDC( ret->handle );
  return ret;
}

void wend( guiInfo* vp ){
  guiInfo* p = (guiInfo*)vp;
  guiState* gui = p->gui;
  ReleaseDC( p->handle, gui->hDC );
  DestroyWindow( p->handle );
  DestroyIcon( gui->wc.hIcon );
  if( gui->title )
    memfree( gui->title );
  if( p->gui )
    memfree( p->gui );
  memfree( p );
}
bool weventLoop( guiInfo* vp ){
  guiInfo* p = (guiInfo*)vp;
  guiState* gui = p->gui;
  MSG msg;
  while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )){
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
  return !gui->quit;
}






LONG WINAPI eventLoop( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ){
  guiInfo* p = (guiInfo*)GetWindowLongPtr( hWnd, 0 );
  if( !p ){
    if( WM_CREATE == uMsg ){
      static bool done = false;
      if( !done ){
	done = true;
	u32 pages[] = { 0x0001, 0x000C, 0x000D, 0x0020, 0x0084, 0x008C };
	const u32 numpages = sizeof( pages ) / sizeof( pages[ 0 ] );
	newa( rid, RAWINPUTDEVICE, numpages );

	for( u32 i = 0; i < numpages; ++i ){
	  rid[ i ].usUsagePage = pages[ i ];
	  rid[ i ].usUsage = 0;
	  rid[ i ].dwFlags = RIDEV_INPUTSINK | RIDEV_PAGEONLY;
	  rid[ i ].hwndTarget = hWnd;
	}     
	if( !RegisterRawInputDevices( rid, numpages,
				      sizeof( RAWINPUTDEVICE ) ) )
	  die( "Device input registration failed" );
	memfree( rid );
      }
      return 0;
    }else
      return (LONG)DefWindowProc( hWnd, uMsg, wParam, lParam );
  }
  guiState* gui = p->gui;
  RECT r;
  switch( uMsg ){
  case WM_INPUT:
    if( !GET_RAWINPUT_CODE_WPARAM( wParam ) ){
      u32 rsz;
      if( (u32)-1 == GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, NULL, 
				      &rsz, sizeof( RAWINPUTHEADER ) ) )
	die( "GetRawInputData failed." );
      RAWINPUT* rinp = mem( rsz );
      if( (u32)-1 == GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, rinp, 
				      &rsz, sizeof( RAWINPUTHEADER ) ) )
	die( "GetRawInputData failed." );
      parseInput( rinp, rsz );
      memfree( rinp );
    }
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
  case WM_SIZE:
    GetClientRect( hWnd, &r );
    p->width = r.right - r.left;
    p->height = r.bottom - r.top;
    plvkTickRendering( state.vk );
    return 0;
  case WM_MOVE:
    p->x = LOWORD( lParam );
    p->y = HIWORD( lParam );
    r.left = p->x;
    r.top = p->y;
    r.right = r.left + p->width;
    r.bottom = r.top + p->height;
    AdjustWindowRect( &r, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS
		      | WS_CLIPCHILDREN, FALSE );
    p->x = r.left;
    p->y = r.top;
    return 0;
  case WM_CLOSE:
    gui->quit = 1;
    return 0;
  }
  if( uMsg == WM_GETMINMAXINFO ||
      uMsg == WM_NCCALCSIZE )
    plvkTickRendering( state.vk );
  return (LONG)DefWindowProc( hWnd, uMsg, wParam, lParam );
}

void guiShow( guiInfo* p ){
  ShowWindow( p->handle, SW_SHOW );
}

void guiHide( guiInfo* p ){
  ShowWindow( p->handle, SW_HIDE );
}

