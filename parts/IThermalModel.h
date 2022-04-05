/*
	IThermalModel.h - Interface class for a thermal model of a component.
	I/O is very simple,

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
class IThermalModel
{
	public:
		static constexpr float C_TO_KELVIN = 273.15f;
		virtual float OnCycle(float f_dT/*time delta*/, float fP /*power in*/) = 0;

		void SetExtrusionRate(float f_vEx) {m_fvEx = f_vEx;};

	protected:
		float m_fAmbientK = 20.0f + C_TO_KELVIN;
		float m_fvEx = 0;
};
