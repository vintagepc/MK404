/*
	Model_Nozzle.h - Thermal model for a nozzle.

	Based on the reference provided by XPila at https://github.com/XPila/thermreg
	Adapted for MK404 in 2022 by VintagePC <https://github.com/vintagepc/>

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

#include "IThermalModel.h"

class ModelNozzle: public IThermalModel
{
	public:
		float OnCycle(float f_dT, float fP) override;

	private:
		float m_C =  6.0;			// [J/K] total heat capacity of entire heat block
		float m_R = 24.5;			// [K/W] absolute thermal resistance between heat block and ambient
		float m_Ch = 2.2;			// [J/K] total heat capacity of heater
		float m_Rh = 2.0;			// [K/W] absolute thermal resistance between heater and heat block
		float m_Cs = 0.34;			// [J/K] total heat capacity of sensor
		float m_Rs = 10.0;			// [K/W] absolute thermal resistance between sensor and heat block
		float m_T = m_fAmbientK;			// [K] heat block temperature (avg)
		float m_Th = m_T;			// [K] heater temperature (avg)
		float m_Ts = m_T;			// [K] sensor temperature (avg)
		float m_Ex = 0.2;			// [J/mm] extrussion energy factor
};
