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
// Vulkan interface.                                                          //
////////////////////////////////////////////////////////////////////////////////

// Scores and picks gpus to pick one, unless whichGPU is a positive, in which case it picks that one.
plvkStatep plvkInit( s32 whichGPU, u32 debugLevel, char* title, int x, int y,
	       int width, int height );
void plvkEnd( plvkStatep vk );  

void draw( void );

void plvkPrintGPUs( void );
void plvkPrintInitInfo( void );

bool plvkeventLoop( plvkStatep p );

void plvkShow( plvkStatep g );
void plvkHide( plvkStatep g );
