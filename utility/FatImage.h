/*
	FatImage.h - Quick and dirty utility class to make crude
	(and probably not-officially-conforming) empty FAT32 images
	for use with the SD card simulator.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

 	This file is part of MK3SIM.

	MK3SIM is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MK3SIM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MK3SIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>

using namespace std;

class FatImage
{
	public:

		enum class Size
		{
			M32 = 32,
			M64 = 64,
			M128 = 128,
			M256 = 256,
			M512 = 512,
			G1 = 1024,
			G2 = 2048
		};

		static bool MakeFatImage(string strFile, string strSize);

		static vector<string> GetSizes()
		{
			vector<string> strSize;
			for(auto it = NameToSize.begin(); it != NameToSize.end(); ++it)
				strSize.push_back(it->first);
			return strSize;
		}

	private:

		typedef union
		{
			uint32_t all;
			uint8_t bytes[4];
		} ui32_cvt_ui8;

		static inline constexpr uint32_t Sector2Bytes(uint32_t val) { return val<<9; } // <<9 = 512 bytes/sector.
		static inline constexpr uint32_t Byte2Sector(uint32_t val) { return val>>9; } // <<9 = 512 bytes/sector.

		static constexpr uint32_t FirstFATAddr = 0x4000;

		static inline uint8_t GetSectorsPerCluster(Size imgSize) { return imgSize>Size::M256 ? 8 : 1; }

		static uint32_t GetSizeInBytes(Size imgSize) { return ((uint32_t)imgSize)<<20; } // 20 = 1024*1024

		static uint32_t GetSecondFatAddr(Size imgSize) {return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize)));}

		static uint32_t GetDataStartAddr(Size imgSize) { return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize))<<1); } // <<10 = 2*512, 2*bytespersector.

		static const uint32_t SectorsPerFat(Size size)
		{
			switch (size)
			{
				case Size::M32:
					return 505;
				case Size::M64:
					return 1009;
				case Size::M128:
					return 2017;
				case Size::M256:
					return 4033;
				case Size::M512:
					return 1022;
				case Size::G1:
					return 2044;
				case Size::G2:
					return 4088;
			}
			return 0;
		};

		static const map<string, Size> NameToSize;

		static const uint8_t FAT32[];
		static const uint8_t FATHeader[];
		static const uint8_t FSInfo_1[];
		static const uint8_t FSInfo_2[];
		static const uint8_t FSInfo_3[];
		static const uint8_t DataRegion[];


};
