/*

	EnabledType.h - Enum for the enabled/disabled options

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

class EnabledType
{
	public:

		enum class Type_t
		{
			NotSet,
			Enabled,
			Disabled
		};

		static inline std::vector<std::string> GetOpts()
		{
			static const std::vector<std::string> v {
				"enabled",
				"disabled"
			};
			return v;
		}

		static const std::map<std::string, Type_t>& GetNameToType()
		{
			static const std::map<std::string, Type_t> m {
				{"",Type_t::NotSet},
				{"enabled",Type_t::Enabled},
				{"disabled",Type_t::Disabled},
			};
			return m;
		};

};
