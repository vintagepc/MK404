/*
	FatImage.cpp - Quick and dirty utility class to make crude
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

#include "FatImage.h"
#include "gsl-lite.hpp"
#include <cstdint>      // for perror
#include <cstring>
#include <fstream> // IWYU pragma: keep
#include <iostream>
#include <vector>       // for vector

// const map<FatImage::Size, uint32_t>FatImage::SectorsPerFat =
// {
// 	make_pair(Size::M32, 505),
// 	make_pair(Size::M64, 1009),
// 	make_pair(Size::M128, 2017),
// 	make_pair(Size::M256, 4033),
// 	make_pair(Size::M512, 1022),
// 	make_pair(Size::G1, 2044),
// 	make_pair(Size::G2, 4088),
// };

const uint8_t FatImage::_FAT32[] = {
	0xEB,0x58,0x90,0x6D,0x6B,0x66,0x73,0x2E,0x66,0x61,0x74,0x00,0x02,0x01,0x20,0x00,
	0x02,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x20,0x00,0x40,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x02,0x00,0xF1,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
	0x01,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x80,0x00,0x29,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x46,0x41,0x54,0x33,0x32,0x20,0x20,0x20,0x0E,0x1F,0xBE,0x77,0x7C,0xAC,
	0x22,0xC0,0x74,0x0B,0x56,0xB4,0x0E,0xBB,0x07,0x00,0xCD,0x10,0x5E,0xEB,0xF0,0x32,
	0xE4,0xCD,0x16,0xCD,0x19,0xEB,0xFE
};

const uint8_t FatImage::_FATHeader[] = {0xF8,0xFF,0xFF,0x0F,0xFF,0xFF,0xFF,0x0F,0xF8,0xFF,0xFF,0x0F};
const uint8_t FatImage::_FSInfo_1[] = { 0x55, 0xAA, 0x52, 0x52, 0x61, 0x41 };
const uint8_t FatImage::_FSInfo_2[] = { 0x72, 0x72, 0x41, 0x61, 0xFF, 0xFF, 0xFF, 0xFF, 0x02};
const uint8_t FatImage::_FSInfo_3[] = {0x55, 0xAA};
const uint8_t FatImage::_DataRegion[] = { 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x08,0x00,0x00,0x57,0x49,
											0xCD,0x50,0xCD,0x50,0x00,0x00,0x57,0x49,0xCD,0x50};


const std::map<std::string, FatImage::Size>& FatImage::GetNameToSize()
{
	static const std::map<std::string, FatImage::Size> m {
		{"32M",FatImage::Size::M32},
		{"64M",FatImage::Size::M64},
		{"128M",FatImage::Size::M128},
		{"256M",FatImage::Size::M256},
		{"512M",FatImage::Size::M512},
		{"1G",FatImage::Size::G1},
		{"2G",FatImage::Size::G2}
	};
	return m;
};

uint8_t FatImage::GetSectorsPerCluster(Size imgSize)
{
	return imgSize>Size::M256 ? 8 : 1;
}

uint32_t FatImage::GetSizeInBytes(Size imgSize)
{
	return static_cast<uint32_t>(imgSize)<<20u; // 20 = 1024*1024
}

uint32_t FatImage::GetSecondFatAddr(Size imgSize)
{
	return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize)));
}

uint32_t FatImage::GetDataStartAddr(Size imgSize)
{
	return FirstFATAddr + (Sector2Bytes(SectorsPerFat(imgSize))<<1u); // <<10 = 2*512, 2*bytespersector.
}

uint32_t FatImage::SectorsPerFat(Size size)
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

std::vector<std::string> FatImage::GetSizes()
{
	std::vector<std::string> strSize;
	for(auto &c : GetNameToSize())
	{
		strSize.push_back(c.first);
	}
	return strSize;
}

bool FatImage::MakeFatImage(const std::string &strFile, const std::string &strSize)
{
	FatImage::Size size = GetNameToSize().at(strSize);
	uint32_t uiSize = GetSizeInBytes(size);

	std::ofstream fsOut(strFile, fsOut.binary);
	if (!fsOut.is_open())
	{
		std::cerr << "Failed to open output file\n";
		return false;
	}
	std::vector<uint8_t> data;

	// Write main FAT block.
	gsl::span<const uint8_t> FAT32 (_FAT32);
	data.insert(data.end(), FAT32.begin(), FAT32.end());

	// Set setors per cluster.
	data[0x0D] = GetSectorsPerCluster(size);

	// set sectors and sectors per fat.
	uint32_t uiSector = Byte2Sector(uiSize);
	std::vector<uint8_t> uiSectBytes = {0,0,0,0};

	std::memcpy(uiSectBytes.data(), &uiSector, 4);

	for (int i=0; i<4; i++)
	{
		data[0x20+i] = uiSectBytes.at(i);
	}

	uiSector = SectorsPerFat(size);
	std::memcpy(uiSectBytes.data(), &uiSector, 4);

	for (int i=0; i<4; i++)
	{
		data[0x24+i] = uiSectBytes.at(i);
	}


	gsl::span<const uint8_t> FSInfo_1 (_FSInfo_1);
	gsl::span<const uint8_t> FSInfo_2 (_FSInfo_2);
	gsl::span<const uint8_t> FSInfo_3 (_FSInfo_3);

	// Copy the FS info signature
	data.resize(0x1FE);
	data.insert(data.end(),FSInfo_1.begin(), FSInfo_1.end());

	data.resize(0x3E4);
	data.insert(data.end(),FSInfo_2.begin(),FSInfo_2.end());

	data.resize(0x3FE);
	data.insert(data.end(), FSInfo_3.begin(), FSInfo_3.end());


	// Second copy of boot record @ 0xC00
	data.resize(0xC00);
	data.insert(data.end(), data.begin(), data.begin()+0x200);

	gsl::span<const uint8_t> FATHeader (_FATHeader);
	// Copy fat header(s)
	data.resize(FirstFATAddr);
	data.insert(data.end(), FATHeader.begin(), FATHeader.end());

	data.resize(GetSecondFatAddr(size));
	data.insert(data.end(), FATHeader.begin(), FATHeader.end());

	// Data start.
	gsl::span<const uint8_t> DataRegion (_DataRegion);
	data.resize(GetDataStartAddr(size));
	data.insert(data.end(), DataRegion.begin(), DataRegion.end());

	fsOut.write(reinterpret_cast<char*>(data.data()),data.size()); //NOLINT - my kingdom for an unsigned char fstream...

	fsOut.seekp(uiSize-1);
	fsOut.put(0);
	if(!fsOut)
	{
		std::cerr << "Failed to write full file to disk... " <<fsOut.tellp() << '\n';
		fsOut.close();
		return false;
	}
	else
	{
		std::cout << "Wrote " << fsOut.tellp() << " bytes to SD image.";
	}


	fsOut.close();
	return true;
}
