/*
	MMUSideband.h - Sideband control codes for MMU board in standalone mode

	Copyright 2023 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK404.

	MK404 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK404 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK404.  If not, see <http://www.gnu.org/licenses/>.
 */

enum MMUSideband {
	RESET = 'R', // Reset line Printer -> MMU was released.
	FS_AUTO_SET = 'F', // MMU -> Printer signalling the filament has been pushed  past 40cm by the pulley
	FS_AUTO_CLEAR = 'f', // MMU -> Printer signalling the filament has been retracted below 40cm by the pulley
};
