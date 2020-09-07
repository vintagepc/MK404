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
#include "png.hpp"
#include <GL/glew.h> //NOLINT
#include <GL/freeglut_std.h>
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
			RegisterAction("Snapshot", "Takes a snap of the current GL rendering", ActTakeSnapshot, {ArgType::String});
			RegisterAction("SnapRect", "Takes a snap a region (file,x,y,w,h)", ActTakeSnapshotArea, {ArgType::String,ArgType::Int,ArgType::Int,ArgType::Int,ArgType::Int});
		}

		// Function for running the GL stuff inside the GL context.
		void OnDraw()
		{
			auto width = 500;//glutGet(GLUT_WINDOW_WIDTH);
			auto height = 324;//glutGet(GLUT_WINDOW_HEIGHT);
			if (m_iState == St_Queued)
			{
				m_iState = St_Busy;
				if (m_vBuffer.size()!=(4u*width*height))
				{
					m_vBuffer.resize(4u*width*height,0);
				}
				switch (m_iAct.load())
				{
					case ActCheckPixel:
					{
						uint8_t uiTmp[4] {0};
						glReadPixels(m_x.load(), m_y.load(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &uiTmp);
						m_color = uiTmp[0]<<24U | uiTmp[1] << 16U | uiTmp[2] <<8U | uiTmp[3];
						m_iState = St_Done;
					}
					break;
					case ActTakeSnapshot:
					{
						m_w = width;
						m_h = height;
					}
					/* FALLTHRU */
					case ActTakeSnapshotArea:
					{
						WritePNG(width, height, m_iAct==ActTakeSnapshotArea);
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

		bool WritePNG(int width, int height, bool bRegion)
		{
			auto w = m_w.load(), h = m_h.load();
			auto iPixCt = w*h;
			if (bRegion)
			{
				glReadPixels(m_x,(height-m_y)-h,w, h, GL_BGRA, GL_UNSIGNED_BYTE, m_vBuffer.data());
			}
			else
			{
				glReadPixels(0,0,width, height, GL_BGRA, GL_UNSIGNED_BYTE, m_vBuffer.data());

			}
			// std::ofstream fsOut;
			// fsOut.open(m_strFile + ".ppm");
			// if (!fsOut.is_open())
			// {
			// 	return false;
			// }
			// fsOut << "P3\n" << std::dec << m_w << " " << m_h << "\n255\n";
			// for (auto i = 0u; i<iPixCt; i++)
			// {
			// 	fsOut << std::to_string(m_vBuffer.at(i)) << ' ';
			// }
			// fsOut << '\n';
			// fsOut.close();
			png::image<png::rgb_pixel,png::solid_pixel_buffer<png::rgb_pixel>> img(w, h);
			size_t i = 0,y=0;
			while(i<iPixCt)
			{
				img[m_bVFlip?(h-y-1):y][i%w] = png::rgb_pixel(m_vBuffer.at((4*i)+2),m_vBuffer.at((4*i)+1),m_vBuffer.at(4*i));
				i++;
				if(i%w==0)	y++;
			}
			img.write(m_strFile.c_str());
			return true;
		}

		// bool WritePPM()
		// {
		// 	glReadPixels(0,0,m_w, m_h, GL_RGB, GL_UNSIGNED_BYTE, m_vBuffer.data());
		// 	std::ofstream fsOut;
		// 	fsOut.open(m_strFile);
		// 	if (!fsOut.is_open())
		// 	{
		// 		return false;
		// 	}
		// 	fsOut << "P3\n" << std::dec << m_w << " " << m_h << "\n255\n";
		// 	for (auto &v : m_vBuffer)
		// 	{
		// 		fsOut << std::to_string(v) << ' ';
		// 	}
		// 	fsOut << '\n';
		// 	fsOut.close();
		// 	return true;
		// }

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
				case ActTakeSnapshotArea:
				{
					bool bIsArea = iAct == ActTakeSnapshotArea;
					if (m_iState == St_Idle)
					{
						m_strFile = vArgs.at(0) + ".png";
						if (bIsArea)
						{
							m_x = std::stoi(vArgs.at(1));
							m_y = std::stoi(vArgs.at(2));
							m_w = std::stoi(vArgs.at(3));
							m_h = std::stoi(vArgs.at(4));
						}
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
			ActTakeSnapshot,
			ActTakeSnapshotArea
		};
		enum ActState
		{
			St_Idle,
			St_Queued,
			St_Busy,
			St_Done
		};
		std::string m_strFile;
		std::atomic_int m_iAct {-1};
		std::atomic_uint32_t m_x{0}, m_y{0}, m_h{0}, m_w{0}, m_color{0};
		std::atomic_int m_iState {St_Idle};
		std::vector<uint8_t> m_vBuffer{};
		bool m_bVFlip = true;
};
