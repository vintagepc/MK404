/*
	GLHelper.cpp - Lightweight helper for testing the GL drawing of parts.
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

#include "GLHelper.h"

#include "IScriptable.h"
#include "Scriptable.h"
#include <GL/glew.h> //NOLINT
#include <GL/freeglut_std.h>
#include <atomic>
#ifdef SUPPORTS_LIBPNG
//#include <exception>               // for exception
#include <image.hpp>               // NOLINT for image
#include <image_info.hpp>
#include <png.h>                   // for png_create_write_struct, png_destr...
#include <rgb_pixel.hpp>           // NOLINT for rgb_pixel, basic_rgb_pixel
#include <solid_pixel_buffer.hpp>  // NOLINT for solid_pixel_buffer
#endif // SUPPORTS_LIBPNG
#include <chrono>
#include <ctime>
#include <fstream> // IWYU pragma: keep
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream> // IWYU pragma: keep
#include <string>
#include <vector>
// IWYU pragma: no_include <bits/exception.h>


GLHelper::GLHelper(const std::string &strName, bool bIsPrimary):Scriptable(strName)
{
	RegisterAction("CheckPixel","Checks the pixel color at the given position matches specified (x,y,RGBA).",ActCheckPixel, {ArgType::uint32,ArgType::uint32, ArgType::uint32});
	RegisterAction("Snapshot", "Takes a snap of the current GL rendering", ActTakeSnapshot, {ArgType::String});
	RegisterAction("SnapRect", "Takes a snap a region (file,x,y,w,h)", ActTakeSnapshotArea, {ArgType::String,ArgType::Int,ArgType::Int,ArgType::Int,ArgType::Int});
	RegisterActionAndMenu("AutoSnap", "Takes a snap of the current GL rendering and gives it the current date/time.", ActTakeSnapDT);
	//RegisterActionAndMenu("AutoSnapLCD", "Takes a snap of the current LCD window and gives it the current date/time.", ActTakeSnapLCD);
	if (bIsPrimary) {
		RegisterKeyHandler('S',"Take a snapshot of the LCD");
	}
}

// Function for running the GL stuff inside the GL context.
void GLHelper::OnDraw()
{
	if (m_iState == St_Queued2)
	{
		auto width = glutGet(GLUT_WINDOW_WIDTH);
		auto height = glutGet(GLUT_WINDOW_HEIGHT);
		m_iState = St_Busy;
		if (m_vBuffer.size()!=(4u*width*height))
		{
			m_vBuffer.resize(4u*width*height,0);
		}
		switch (m_iAct.load())
		{
			case ActCheckPixel:
			{
				uint32_t uiTmp[4] {0};
				glReadPixels(m_x,(height-m_y)-1u, 1, 1, GL_RGBA, GL_UNSIGNED_INT, &uiTmp);
				m_color = uiTmp[0]<<24U | (uiTmp[1]&0xFFu) << 16U | (uiTmp[2]&0xFFu) <<8U | (uiTmp[3]&0xFFu);
				m_iState = St_Done;
			}
			break;
			case ActTakeSnapshot:
			case ActTakeSnapDT:
			{
				m_w = width;
				m_h = height;
			}
			/* FALLTHRU */
			case ActTakeSnapshotArea:
			case ActTakeSnapLCD:
			{
				if (m_iAct.load() == ActTakeSnapLCD) // Account for LCD scaling
				{
					m_h = static_cast<float>(m_h)*(static_cast<float>(width)/static_cast<float>(m_w));
					m_w = width;
				}
				WritePNG(width, height, (m_iAct == ActTakeSnapshotArea || m_iAct == ActTakeSnapLCD));
				std::cout << "Wrote: " << m_strFile << '\n';
				if (m_bIsKeySnap) {
					m_bIsKeySnap = false;
					m_iState = St_Idle;
				} else {
					m_iState = St_Done;
				}
			}
			break;
			default:
			{

			}
		}

	}
	if (m_iState == St_Queued)
	{
		m_iState = St_Queued2;
	}
}

void GLHelper::OnKeyPress(const Key& key)
{
	switch (key)
	{
		case 'S':
		{
			std::cout << "LCD Snapshot toggled!\n";
			m_bIsKeySnap = true;
			// Args are scaled based on window width in the routine.
			ProcessAction(ActTakeSnapLCD,{"","0","0","500","164"});
		}
		break;
	}
}

bool GLHelper::WritePNG(int width, int height, bool bRegion)
{
	auto w = m_w.load(), h = m_h.load();
	if (bRegion)
	{
		glReadPixels(m_x,(height-m_y)-h,w, h, GL_BGRA, GL_UNSIGNED_BYTE, m_vBuffer.data());
	}
	else
	{
		glReadPixels(0,0,width, height, GL_BGRA, GL_UNSIGNED_BYTE, m_vBuffer.data());

	}
#ifdef SUPPORTS_LIBPNG
	auto iPixCt = w*h;
	png::image<png::rgb_pixel,png::solid_pixel_buffer<png::rgb_pixel>> img(w, h);
	size_t i = 0,y=0;
	while(i<iPixCt)
	{
		img[m_bVFlip?(h-y-1):y][i%w] = png::rgb_pixel(m_vBuffer.at((4*i)+2),m_vBuffer.at((4*i)+1),m_vBuffer.at(4*i));
		i++;
		if(i%w==0)	y++;
	}
	img.write(m_strFile);
#else
	std::cerr << "TODO: Sorry, libPNG is not implemented. Snapshots cannot be saved.\n";
#endif // SUPPORTS_LIBPNG
	return true;
}
// Useful for debugging as it's a very simple format.
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

IScriptable::LineStatus GLHelper::ProcessAction(unsigned int iAct, const std::vector<std::string> &vArgs)
{
	switch (iAct)
	{
		case ActCheckPixel:
		{
			if (m_iState == St_Idle)
			{
				// Dispatch check.
				m_x = std::stoul(vArgs.at(0));
				m_y = std::stoul(vArgs.at(1)); // Work from the top down so new additions at the bottom of the window don't mess up existing tests.
				m_iAct = iAct;
				m_iState = St_Queued;
			}
			else if (m_iState == St_Done)
			{
				m_iState = St_Idle;
				std::stringstream strVal;
				strVal << "0x" << std::setw(8) << std::setfill('0') << std::hex << m_color.load();
				std::cout << "Actual pixel color: " << strVal.str() << '\n';
				// Check agains the first n chars, so you can ignore alpha.
				bool bMatch = vArgs.at(2) == strVal.str().substr(0,vArgs.at(2).length());
				if (!bMatch)
				{
					return LineStatus::Timeout;
				}
				else
				{
					return LineStatus::Finished;
				}

			}
			return LineStatus::HoldExec;
		}
		break;
		case ActTakeSnapshot:
		case ActTakeSnapshotArea:
		case ActTakeSnapDT:
		case ActTakeSnapLCD:
		{
			bool bIsArea = (iAct == ActTakeSnapshotArea || iAct == ActTakeSnapLCD);
			if (m_iState == St_Idle)
			{
				if (iAct == ActTakeSnapDT || iAct == ActTakeSnapLCD) {
					auto tNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
					m_strFile = std::ctime(&tNow);
					m_strFile = m_strFile.substr(0,m_strFile.size()-1);// strip newline.
					m_strFile += ".png";
				} else {
					m_strFile = vArgs.at(0) + ".png";
				}
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
				return LineStatus::Finished;
			}
			return LineStatus::HoldExec; // Pauses primary board execution until finished.
		}
		break;
		default:
			return LineStatus::Unhandled;
	}
}
