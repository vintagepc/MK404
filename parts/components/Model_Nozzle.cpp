/*
	Model_Nozzle.cpp - Thermal model for a nozzle.

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

#include "Model_Nozzle.h"

float ModelNozzle::OnCycle(float f_dT, float fP)
{
	float E = m_C * m_T; //[J] total heat energy stored in heater block
	float Eh = m_Ch * m_Th; //[J] total heat energy stored in heater
	float Es = m_Cs * m_Ts; //[J] total heat energy stored in sensor
	float Pl = (m_T - m_fAmbientK) / m_R; //[W] power from heater block to ambient (leakage power)
	float Ph = (m_Th - m_T) / m_Rh; //[W] power from heater to heater block
	float Ps = (m_T - m_Ts) / m_Rs; //[W] power from heater block to sensor
	float Ehd = ((fP - Ph) * f_dT); //[J] heater energy increase
	float Ed = (Ph - (Pl + Ps)) * f_dT; //[J] heater block energy increase
	float Esd = (Ps * f_dT); //[J] sensor energy increase
	float Ex = (m_Ex * m_fvEx * f_dT);
	Ed -= Ex;
	Eh += Ehd; //[J] heater result total heat energy
	E += Ed; //[J] heater block result total heat energy
	Es += Esd; //[J] sensor result total heat energy
	m_Th = Eh / m_Ch; //[K] result heater temperature
	m_T = E / m_C; //[K] result heater block temperature
	m_Ts = Es / m_Cs; //[K] result sensor temperature
	return m_Ts - C_TO_KELVIN;
}

