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



#define MAX_INPUTS 32
#define MAX_AXES 256
#define MAX_BUTTONS 1024

#define DEADZONE 0.07

typedef struct hasht hasht;

typedef struct axis{
  u32 minAxis;
  u32 usagePage;
  s32 minVal;
  s32 maxVal;
  f32 val;
} axis;


typedef enum gpuInputDeviceType{
  GPU_KEYBOARD = 1,
  GPU_MOUSE = 2,
  GPU_OTHER = 3
} gpuInputDeviceType;


typedef struct inputDevice{
  gpuInputDeviceType type;
  u32 usagePage;
  u32 minButton;
  u32 numButtons;
  u32 numAxes;
  bool* buttons;
  axis* axes;
} inputDevice;

typedef struct gpuInputDevice{
  u32 numButtons;
  u32 numAxes;
  u32 boffset;
  u32 aoffset;
} gpuInputDevice;  
  

typedef struct inputDeviceBuffers {
  u32 numDevices;
  u32 _unused1;
  f32 axes[ MAX_AXES ];
  f32 buttons[ MAX_BUTTONS ];
  u32 types[ MAX_INPUTS ];
  gpuInputDevice devices[ MAX_INPUTS ];
} inputDeviceBuffers;

void parseInput( RAWINPUT* rinp, u64 arsz );
void updateInputBuffers( hasht* devs, inputDeviceBuffers* devbs );
