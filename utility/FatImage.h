/*
	FatImage.h - Quick and dirty utility class to make crude
	(and probably not-officially-conforming) empty FAT32 images
	for use with the SD card simulator.

	Copyright 2020 VintagePC <https://github.com/vintagepc/>

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

#pragma once

#include <cstdint>  // for uint32_t, uint8_t
#include <map>       // for _Rb_tree_const_iterator, map
#include <string>    // for string
#include <utility>   // for pair
#include <vector>    // for vector

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

		static bool MakeFatImage(const std::string &strFile, const std::string &strSize);

		static std::vector<std::string> GetSizes()
		{
			std::vector<std::string> strSize;
			for(auto &c : GetNameToSize())
			{
				strSize.push_back(c.first);
			}
			return strSize;
		}

	private:
		static inline constexpr uint32_t Sector2Bytes(uint32_t val) { return val<<9; } // <<9 = 512 bytes/sector.
		static inline constexpr uint32_t Byte2Sector(uint32_t val) { return val>>9; } // <<9 = 512 bytes/sector.

		static constexpr uint32_t FirstFATAddr = 0x4000;

		static inline uint8_t GetSectorsPerCluster(Size imgSize) { return imgSize>Size::M256 ? 8 : 1; }

		static uint32_t GetSizeInBytes(Size imgSize) { return static_cast<uint32_t>(imgSize)<<20; } // 20 = 1024*1024

		static uint32_t GetSecondFatAddr(Size imgSize) {return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize)));}

		static uint32_t GetDataStartAddr(Size imgSize) { return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize))<<1); } // <<10 = 2*512, 2*bytespersector.

		static uint32_t SectorsPerFat(Size size)
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

		static const std::map<std::string, Size>& GetNameToSize();

		static const uint8_t _FAT32[];
		static const uint8_t _FATHeader[];
		static const uint8_t _FSInfo_1[];
		static const uint8_t _FSInfo_2[];
		static const uint8_t _FSInfo_3[];
		static const uint8_t _DataRegion[];


};
