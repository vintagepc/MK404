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

		static std::vector<std::string> GetSizes();

		static bool MakeFatImage(const std::string &strFile, const std::string &strSize);

	private:
		static inline constexpr uint32_t Sector2Bytes(uint32_t val) { return val<<9u; } // <<9 = 512 bytes/sector.
		static inline constexpr uint32_t Byte2Sector(uint32_t val) { return val>>9u; } // <<9 = 512 bytes/sector.

		static constexpr uint32_t FirstFATAddr = 0x4000;

		static uint8_t GetSectorsPerCluster(Size imgSize);

		static uint32_t GetSizeInBytes(Size imgSize);

		static uint32_t GetSecondFatAddr(Size imgSize);

		static uint32_t GetDataStartAddr(Size imgSize);

		static uint32_t SectorsPerFat(Size size);

		static const std::map<std::string, Size>& GetNameToSize();

		static const uint8_t _FAT32[];
		static const uint8_t _FATHeader[];
		static const uint8_t _FSInfo_1[];
		static const uint8_t _FSInfo_2[];
		static const uint8_t _FSInfo_3[];
		static const uint8_t _DataRegion[];


};
