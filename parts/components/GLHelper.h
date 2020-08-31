/*
	GLHelper.h - Lightweight helper for testing the GL drawing of parts.
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

#include "IScriptable.h"
#include "Scriptable.h"
#include "Util.h"
#include <GL/glew.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class GLHelper: public Scriptable
{
	public:
		GLHelper():Scriptable("GLHelper")
		{
			RegisterAction("CheckPixel","Checks the pixel color at the given position matches specified (x,y,RGBA).",ActCheckPixel, {ArgType::uint32,ArgType::uint32, ArgType::uint32});
			RegisterAction("Snapshot", "Takes a PPM snapshot of the current GL rendering", ActTakeSnapshot, {ArgType::String});
		}

		// Tell the helper the window height so coordinates can be from the top left.
		inline void SetWindowHeight(unsigned w, unsigned h)
		{
			m_w = w;
			m_h = h;
			m_vBuffer.resize(w*h*3,0);
		}

		// Function for running the GL stuff inside the GL context.
		void OnDraw()
		{
			if (m_iState.load() == St_Queued)
			{
				switch (m_iAct.load())
				{
					case ActCheckPixel:
					{
						uint8_t uiTmp[4] {0};
						glReadPixels(m_x.load(), m_y.load(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &uiTmp);
						m_color = uiTmp[0]<<24 | uiTmp[1] << 16 | uiTmp[2] <<8 | uiTmp[3];
						m_iState = St_Done;
					}
					break;
					case ActTakeSnapshot:
					{
						WritePPM();
						m_iState = St_Done;
					}
					break;
					default:
					{

					}
				}

			}
		}
	protected:

		bool WritePPM()
		{
			glReadPixels(0,0,m_w, m_h, GL_RGB, GL_UNSIGNED_BYTE, m_vBuffer.data());
			std::ofstream fsOut;
			fsOut.open(m_strFile);
			if (!fsOut.is_open())
			{
				return false;
			}
			fsOut << "P3\n" << std::dec << m_w << " " << m_h << "\n255\n";
			for (auto &v : m_vBuffer)
			{
				fsOut << std::to_string(v) << ' ';
			}
			fsOut << '\n';
			fsOut.close();
			return true;
		}

		LineStatus ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs) override
		{
			switch (iAct)
			{
				case ActCheckPixel:
				{
					if (m_iState == St_Idle)
					{
						// Dispatch check.
						m_x = std::stoul(vArgs.at(0));
						m_y = m_h - std::stoul(vArgs.at(1)); // Work from the top down so new additions at the bottom of the window don't mess up existing tests.
						m_iAct = iAct;
						m_iState = St_Queued;
					}
					else if (m_iState == St_Done)
					{
						m_iState = St_Idle;
						std::stringstream strVal;
						strVal << "0x" << std::setw(8) << std::setfill('0') << std::hex << m_color.load();
						std::cout << "Actual pixel color: " << strVal.str() << '\n';
						if (strVal.str()!=vArgs.at(2))
						{
							return LineStatus::Timeout;
						}
						else
						{
							return LineStatus::Finished;
						}

					}
					return LineStatus::Waiting;
				}
				break;
				case ActTakeSnapshot:
				{
					if (m_iState == St_Idle)
					{
						m_strFile = vArgs.at(0) + ".ppm";
						m_iAct = iAct;
						m_iState = St_Queued;
					}
					else if (m_iState == St_Done)
					{
						m_iState = St_Idle;
						std::cout << "Wrote: " << m_strFile << '\n';
						return LineStatus::Finished;
					}
					return LineStatus::Waiting;
				}
				break;
				default:
					return LineStatus::Unhandled;
			}
		}

		enum Actions
		{
			ActCheckPixel,
			ActTakeSnapshot
		};
		enum ActState
		{
			St_Idle,
			St_Queued,
			St_Done
		};
		std::string m_strFile;
		std::atomic_int m_iAct {-1};
		std::atomic_uint32_t m_x{0}, m_y{0}, m_color{0};
		std::atomic_int m_iState {St_Idle};
		std::vector<uint8_t> m_vBuffer{};
		unsigned int m_h = 0, m_w = 0;
};
