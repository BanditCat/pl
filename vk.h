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

typedef void* plvkStatep;

// Scores and picks gpus to pick one, unless whichGPU is a positive, in which case it picks that one.
void plvkInit( u32 whichGPU, guiInfo* gui, u32 debugLevel );
void plvkEnd( plvkStatep vk );  

void plvkPrintGPUs( void );
#ifdef DEBUG
void plvkPrintInitInfo( void );
#endif		    
