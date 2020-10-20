/*
	PLYExporter.h - PLY export helper for GLPrint

	Copyright 2020 DRracer <https://github.com/DRracer/>

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

#include <string>
#include <vector>

class PLYExporter
{
	public:
		using VF = std::vector<float>; // this is what the HR renderer uses for
		using VI = std::vector<int>; // these are the indices for GL_TRIANGLE_STRIP and triangle counts

		static bool Export(const std::string& strFN, const VF &tri, const VF &triNorm, const VF& triColor, const VI &tStart, const VI &tCount);
};
