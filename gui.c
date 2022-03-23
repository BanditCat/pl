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


#define MAX_BUTTONS 256
void parseInput( RAWINPUT* rinp, u64 arsz ){
  u32 rsz = arsz;
  if( RIM_TYPEHID == rinp->header.dwType ){
    hasht* devices = state.osstate->devices;
    u32 h = hash( &rinp->header.hDevice, sizeof( HANDLE ), devices );
    inputDevice** idevp =
      (inputDevice**)htFindWithHash( devices, &rinp->header.hDevice,
				     sizeof( HANDLE ), NULL, h );
    inputDevice* idev;
    if( (u32)-1 == GetRawInputDeviceInfoW( rinp->header.hDevice,
					   RIDI_PREPARSEDDATA, NULL, &rsz ) )
      die( "GetRawInputDeviceInfoW failed." );
    PHIDP_PREPARSED_DATA ppd = mem( rsz );
    if( (u32)-1 == GetRawInputDeviceInfoW( rinp->header.hDevice,
					   RIDI_PREPARSEDDATA, ppd, &rsz ) )
      die( "GetRawInputDeviceInfoW failed." );
    if( !idevp ){
      idev = newe( inputDevice );
      htAddWithHash( devices, &rinp->header.hDevice, sizeof( HANDLE ),
		     &idev, sizeof( inputDevice* ), h );
      
      
      HIDP_CAPS caps;
      if( HIDP_STATUS_SUCCESS != HidP_GetCaps( ppd, &caps ) )
	die( "HidP_GetCaps failed." );
      PHIDP_BUTTON_CAPS bcaps = mem( sizeof( HIDP_BUTTON_CAPS ) *
				     caps.NumberInputButtonCaps );
      u16 bcapsLen = caps.NumberInputButtonCaps;
      if( HIDP_STATUS_SUCCESS != HidP_GetButtonCaps( HidP_Input, bcaps,
						     &bcapsLen, ppd ) )
	die( "HidP_GetButtonCaps failed." );

      u16 vcapsLen = caps.NumberInputValueCaps;
      PHIDP_VALUE_CAPS vcaps = mem( sizeof( HIDP_VALUE_CAPS ) *
				    caps.NumberInputValueCaps );
      if( HIDP_STATUS_SUCCESS != HidP_GetValueCaps( HidP_Input, vcaps,
						    &vcapsLen, ppd ) )
	die( "HidP_GetValueCaps failed." );

      
      idev->usagePage = bcaps->UsagePage;
      idev->minButton = bcaps->Range.UsageMin;
      idev->numButtons = bcaps->Range.UsageMax - bcaps->Range.UsageMin;
      idev->buttons = newae( bool, idev->numButtons );

      idev->numAxes = vcapsLen;
      idev->axes = newae( axis, idev->numAxes );
      for( u32 i = 0; i < idev->numAxes; ++i ){
	idev->axes[ i ].minAxis = vcaps[ i ].Range.UsageMax;
	idev->axes[ i ].usagePage = vcaps[ i ].UsagePage;
	idev->axes[ i ].minVal = vcaps[ i ].LogicalMin;
	idev->axes[ i ].maxVal = vcaps[ i ].LogicalMax;
	if( idev->axes[ i ].maxVal < idev->axes[ i ].minVal ){
	  idev->axes[ i ].minVal = 0;
	  idev->axes[ i ].maxVal = 65536;
	}
      }
      memfree( bcaps );
      memfree( vcaps );
    } else
      idev = *idevp;
    printInt( idev->numAxes ); print( " fx " ); printInt( idev->numButtons );
    endl();
    USAGE usage[ MAX_BUTTONS ];
    unsigned long usageLength = idev->numButtons;
    if( HIDP_STATUS_SUCCESS != HidP_GetUsages( HidP_Input, idev->usagePage,
					       0, usage, &usageLength, ppd,
					       (PCHAR)rinp->data.hid.bRawData,
					       rinp->data.hid.dwSizeHid ) )
      die( "HidP_GetUsages failed." );
    for( u32 i = 0; i < idev->numAxes; ++i ){
      unsigned long value;
      if( HIDP_STATUS_SUCCESS !=
	  HidP_GetUsageValue( HidP_Input, idev->axes[ i ].usagePage, 0,
			      idev->axes[ i ].minAxis,
			      &value, ppd, (PCHAR)rinp->data.hid.bRawData,
			      rinp->data.hid.dwSizeHid ) )
	die( "HidP_GetUsageValue failed." );
      idev->axes[ i ].val = (f32)( (long)value - idev->axes[ i ].minVal ) /
	(f32)( idev->axes[ i ].maxVal - idev->axes[ i ].minVal );
      idev->axes[ i ].val = idev->axes[ i ].val * 2 - 1;
    }

    bool bstates[ MAX_BUTTONS ];
    memset( bstates, 0, sizeof( bstates ) );
    for( u32 i = 0; i < idev->numButtons; i++ )
      idev->buttons[ i ] = false;
    for( u32 i = 0; i < usageLength; i++)
      idev->buttons[ usage[ i ] - idev->minButton ] = true;
    memfree( ppd );
  
  }
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

