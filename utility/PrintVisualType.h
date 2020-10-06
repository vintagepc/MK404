/*

    PrintVisualType.h - Enum for the type of print visualization.

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

#include <map>
#include <string>
#include <vector>

class PrintVisualType
{
	public:

		enum
		{
			LINE,
			QUAD,
			QUAD_HIGHRES,
			TUBE,
			TUBE_HIGHRES
		};

		static inline std::vector<std::string> GetOpts()
		{
			std::vector<std::string> strTypes;
			for(auto &c : GetNameToType())
			{
				strTypes.push_back(c.first);
			}
			return strTypes;
		}

		static const std::map<std::string, unsigned int>& GetNameToType()
		{
			static const std::map<std::string, unsigned int> m {
				{"Line",LINE},
				{"Quad_Avg",QUAD},
				{"Quad_HR",QUAD_HIGHRES},
				{"Tube_Avg",TUBE},
				{"Tube_HR",TUBE_HIGHRES},
			};
			return m;
		};

};
