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
// Input.                                                                     //
////////////////////////////////////////////////////////////////////////////////

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>

#include "vkutil.h"
#include "os.h"
#include "util.h"

void parseMouse( RAWINPUT* rinp ){
  hasht* devices = state.osstate->devices;
  u32 h = hash( &rinp->header.hDevice, sizeof( HANDLE ), devices );
  inputDevice** idevp =
    (inputDevice**)htFindWithHash( devices, &rinp->header.hDevice,
				   sizeof( HANDLE ), NULL, h );
  inputDevice* idev;
  if( !idevp ){
    marc;
    idev = newe( inputDevice );
    htAddWithHash( devices, &rinp->header.hDevice, sizeof( HANDLE ),
		   &idev, sizeof( inputDevice* ), h );
      
    idev->usagePage = 0;
    idev->minButton = 0;
    idev->numButtons = 5;
    idev->buttons = newae( bool, idev->numButtons );
    idev->type = GPU_MOUSE;
    idev->numAxes = 4;
    idev->axes = newae( axis, idev->numAxes );
    marc;
  } else
    idev = *idevp;

  idev->axes[ 0 ].val = (f32)( rinp->data.mouse.lLastX ) / 3.0;
  idev->axes[ 1 ].val = (f32)( -rinp->data.mouse.lLastY ) / 3.0;
  
  u32 mask = 1;
  for( u32 i = 0; i < 5; ++i ){
    if( rinp->data.mouse.usButtonFlags & mask )
      idev->buttons[ i ] = true;
    mask <<= 1;
    if( rinp->data.mouse.usButtonFlags & mask )
      idev->buttons[ i ] = false;
    mask <<= 1;
  }
  if( rinp->data.mouse.usButtonFlags & 1024 )
    idev->axes[ 2 ].val = (f32)( (short)rinp->data.mouse.usButtonData ) / 12.0;
  if( rinp->data.mouse.usButtonFlags & 2048 )
    idev->axes[ 3 ].val = (f32)( (short)rinp->data.mouse.usButtonData ) / 12.0;
  

  printInt( rinp->data.mouse.usButtonFlags );endl();
}
void parseKeyboard( RAWINPUT* rinp ){
  hasht* devices = state.osstate->devices;
  u32 h = hash( &rinp->header.hDevice, sizeof( HANDLE ), devices );
  inputDevice** idevp =
    (inputDevice**)htFindWithHash( devices, &rinp->header.hDevice,
				   sizeof( HANDLE ), NULL, h );
  inputDevice* idev;
  if( !idevp ){
    idev = newe( inputDevice );
    htAddWithHash( devices, &rinp->header.hDevice, sizeof( HANDLE ),
		   &idev, sizeof( inputDevice* ), h );
      
    idev->usagePage = 0;
    idev->minButton = 0;
    idev->numButtons = 256;
    idev->buttons = newae( bool, idev->numButtons );
    idev->type = GPU_KEYBOARD;
    idev->numAxes = 0;
  } else
    idev = *idevp;
    
  idev->buttons[ rinp->data.keyboard.VKey ] = !( rinp->data.keyboard.Flags &
						 1 );
}
void parseInput( RAWINPUT* rinp, u64 arsz ){
  u32 rsz = arsz;
  if( RIM_TYPEMOUSE == rinp->header.dwType ){
    parseMouse( rinp );
  }else if( RIM_TYPEKEYBOARD == rinp->header.dwType ){
    parseKeyboard( rinp );
  }else if( RIM_TYPEHID == rinp->header.dwType ){
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
      marc;
      idev = newe( inputDevice );
      idev->type = GPU_OTHER;
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
      if( fabsolute( idev->axes[ i ].val ) < DEADZONE )
	idev->axes[ i ].val = 0;
      else
	idev->axes[ i ].val *= ( 1.0 / ( 1.0 - DEADZONE ) );
    }

    bool* bstates = newae( bool,  MAX_BUTTONS  );
    memset( bstates, 0, sizeof( bool ) * MAX_BUTTONS );
    for( u32 i = 0; i < idev->numButtons; i++ )
      idev->buttons[ i ] = false;
    for( u32 i = 0; i < usageLength; i++ )
      idev->buttons[ usage[ i ] - idev->minButton ] = true;
    memfree( ppd );
    memfree( bstates );
  }
}

void updateInputBuffers( hasht* devs, inputDeviceBuffers* devbs ){
  u32 numDevices = 0;
  u32 numButtons = 0;
  u32 numAxes = 0;
  for( u32 i = 0; i < htElementCount( devs ); ++i ){
    inputDevice* inp = *( (inputDevice**)htByIndex( devs, i, NULL ) );
    if( ( numButtons + inp->numButtons < MAX_BUTTONS ) &&
	( numAxes + inp->numAxes < MAX_AXES ) &&
	( numDevices < MAX_INPUTS ) ){
      devbs->devices[ numDevices ].numButtons = inp->numButtons;
      devbs->devices[ numDevices ].numAxes = inp->numAxes;
      devbs->devices[ numDevices ].boffset = numButtons;
      devbs->devices[ numDevices ].aoffset = numAxes;
      devbs->types[ numDevices++ ] = inp->type;
      for( u32 j = 0; j < inp->numButtons; ++j ){
	devbs->buttons[ numButtons++ ] = inp->buttons[ j ];
      }
      for( u32 j = 0; j < inp->numAxes; ++j ){
	devbs->axes[ numAxes++ ] = inp->axes[ j ].val;
      }
    }
    if( inp->type == GPU_MOUSE ){
      for( u32 j = 0; j < inp->numAxes; ++j ){
	inp->axes[ j ].val = 0;
      }
    }
  }
  devbs->numDevices = numDevices;
}
