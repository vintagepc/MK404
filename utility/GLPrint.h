/*
	GLPrint.h - Object responsible for print visualization

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

#include <vector>
#include <tuple>

class GLPrint
{
	public:
	GLPrint();

	void Draw();
	void NewCoord(float fX, float fY, float fZ, float fE);

	private:
		std::vector<std::array<float,4>> m_vCoords;
		std::array<float,4> *m_pfSegStart;
		std::array<float,4> m_fSegEnd;
		std::vector<int> m_ivStart = {0};
		std::vector<int> m_ivCount;
		std::vector<float> m_fvDraw = {0,0,0};
		float m_fERetr = 0;
		bool m_bExtruding = false;
};