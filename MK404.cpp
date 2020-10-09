/*
	Einsy.cpp - MK404 printer simulator for the Prusa i3 MK2/3 range.
	Dubbed MK404 as a tribute to their web 404 page. ;-)

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

#include "Config.h"
#include "FatImage.h"                 // for FatImage
#include "KeyController.h"
#include "Macros.h"
#include "PrintVisualType.h"
#include "Printer.h"                  // for Printer, Printer::VisualType
#include "PrinterFactory.h"           // for PrinterFactory
#include "ScriptHost.h"               // for ScriptHost
#include "TelemetryHost.h"
#include "gitversion/version.h"
#include "parts/Board.h"              // for Board
#include "sim_avr.h"                  // for avr_t
#include "tclap/CmdLine.h"            // for CmdLine
#include "tclap/MultiArg.h"           // for MultiArg
#include "tclap/MultiSwitchArg.h"     // for MultiSwitchArg
#include "tclap/SwitchArg.h"          // for SwitchArg
#include "tclap/UnlabeledValueArg.h"  // for UnlabeledValueArg
#include "tclap/ValueArg.h"           // for ValueArg
#include "tclap/ValuesConstraint.h"   // for ValuesConstraint
// NOLINTNEXTLINE - glew has to be included before the freeglut libraries.
#include <GL/glew.h>                  // for glVertex2f, glEnable, glTranslatef
#include <GL/freeglut_std.h>          // for glutGet, glutTimerFunc, glutCre...
#include <GL/freeglut_ext.h>          // for glutSetOption, glutLeaveMainLoop
#include <algorithm>                  // for find
#include <atomic>
#include <csignal>                   // for signal, SIGINT
#include <cstdio>                    // for printf, NULL, fprintf, getchar
#include <cstdlib>                   // for exit
#include <iomanip>
#include <iostream>                   // for operator<<, basic_ostream, '\n'
#include <string>                     // for string, basic_string
#include <utility>                    // for pair
#include <vector>                     // for vector


int window = 0;

std::atomic_int iWinH{0}, iWinW{0};

Printer *printer = nullptr;
Boards::Board *pBoard = nullptr;

bool m_bStopping = false;

bool m_bTestMode = false;

// GL context stuff for FPS counting...

int m_iTic =0, m_iLast = 0, m_iFrCount = 0;


// pragma: LCOV_EXCL_START
// Exit cleanly on ^C
void OnSigINT(int) {
	if (!m_bStopping)
	{
		std::cout << "Caught SIGINT... stopping..." << '\n';
		m_bStopping = true;
		pBoard->SetQuitFlag();
	}
	else
	{
		std::cout << "OK, OK! I get the message!" << '\n';
		exit(2);
	}
}

extern "C" {
	void GLAPIENTRY
	GLErrorCB( GLenum, //source,
				GLenum type,
				GLuint id,
				GLenum severity,
				GLsizei, // length
				const GLchar* message,
				const void*) // userparam )
	{
		std::cerr <<  "GL:";
		if (type == GL_DEBUG_TYPE_ERROR)
		{
			std::cerr << "** GL ERROR **";
		}
		std::cerr << "ID:" << id << " type = " << type << " sev = " << severity << " message = " << message << '\n';
	}
}
// pragma: LCOV_EXCL_STOP

static std::string GetBaseTitle()
{
	static std::string strTitle;
	if (strTitle.empty())
	{
		strTitle += "Prusa i3 MK404 (PRINTER NOT FOUND) ";
		strTitle += version::GIT_TAG_NAME;
		strTitle.push_back('+');
		strTitle+= std::to_string(version::GIT_COMMITS_SINCE_TAG);
	}
	return strTitle;
}

std::atomic_bool bIsQuitting {false};

void displayCB()		/* function called whenever redisplay needed */
{
	if (bIsQuitting || pBoard->GetQuitFlag()) // Stop drawing if shutting down.
	{
		bIsQuitting = true;
		glutLeaveMainLoop();
		return;
	}
	glLoadIdentity();
	glClear(US(GL_COLOR_BUFFER_BIT) | US(GL_DEPTH_BUFFER_BIT));
	int iW = glutGet(GLUT_WINDOW_WIDTH);
	int iH = glutGet(GLUT_WINDOW_HEIGHT);
	printer->Draw();

	m_iFrCount++;
	m_iTic=glutGet(GLUT_ELAPSED_TIME);
	auto iDiff = m_iTic - m_iLast;
	if (iDiff > 1000) {
		int iFPS = m_iFrCount*1000.f/(iDiff);
		m_iLast = m_iTic;
		m_iFrCount = 0;
		std::string strFPS = GetBaseTitle() + " (" +std::to_string(iFPS) + " FPS)";
		glutSetWindowTitle(strFPS.c_str());
	}

	if (pBoard->IsStopped() || pBoard->IsPaused())
	{
		std::string strState = pBoard->IsStopped() ? "Stopped" : "Paused";
		glColor4f(.5,.5,.5,0.5);
		glBegin(GL_QUADS);
			glVertex2f(0,0);
			glVertex2f(iW,0);
			glVertex2f(iW,iH);
			glVertex2f(0,iH);
		glEnd();
		glColor3f(1,0,0);
		glPushMatrix();
			glTranslatef(90,40,0);
			glScalef(0.25,-0.25,1);
			glTranslatef(0,-50,0);
			glRotatef(8.f,0,0,1);
			glPushAttrib(GL_LINE_BIT);
				glLineWidth(5);
				for (auto &i : strState)
				{
					glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,i);
				}
			glPopAttrib();
		glPopMatrix();


	}
	glutSwapBuffers();
}


// pragma: LCOV_EXCL_START
void MouseCB(int button, int action, int x, int y)	/* called on key press */
{
	printer->OnMousePress(button,action,x,y);
}

void MotionCB(int x, int y)
{
	printer->OnMouseMove(x,y);
}
// pragma: LCOV_EXCL_STOP
// gl timer. if the lcd is dirty, refresh display
void timerCB(int i)
{
	if (bIsQuitting)
	{
		return;
	}
	glutSetWindow(window);
	if (iWinH!=glutGet(GLUT_WINDOW_HEIGHT) || iWinW != glutGet(GLUT_WINDOW_WIDTH))
	{
		glutReshapeWindow(iWinW, iWinH);
	}
	glutTimerFunc(20, timerCB, i);
	glutPostRedisplay();
}


void ResizeCB(int w, int h)
{
	std::pair<int,int> winSize = printer->GetWindowSize();
	float fWS = static_cast<float>(w)/static_cast<float>(winSize.first*4);
	float fHS = static_cast<float>(h)/static_cast<float>(winSize.second*4);
	float fScale = std::max(fWS,fHS);
	int iW = 4.f*static_cast<float>(winSize.first)*fScale;
	int iH = 4.f*static_cast<float>(winSize.second)*fScale;
	if (iW!=w || iH !=h)
	{
		iWinH = iH;
		iWinW = iW;
	}
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 10);
	glTranslatef(0, h, 0);
	glScalef(fScale,-fScale,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

int initGL()
{
	// Set up projection matrix
	glutDisplayFunc(displayCB);		/* set window's display callback */
	glutKeyboardFunc(KeyController::GLKeyReceiver);		/* set window's key callback */
	glutMouseFunc(MouseCB);
	glutMotionFunc(MotionCB);
	glutTimerFunc(1000, timerCB, 0);
	glutReshapeFunc(ResizeCB);
#ifndef TEST_MODE
	glEnable(GL_MULTISAMPLE);
#endif
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(.0f, 0.f, 0.f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

#ifndef TEST_MODE
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
#endif

	return 1;
}


int main(int argc, char *argv[])
{
	using TCLAP::SwitchArg;
	using TCLAP::MultiSwitchArg;
	using TCLAP::ValueArg;
	using TCLAP::MultiArg;
	using TCLAP::ValuesConstraint;
	using string = std::string;
	using std::vector;
	signal(SIGINT, OnSigINT);

	TCLAP::CmdLine cmd("MK404 is an 8-bit AVR based 3D printer simulator for firmware debugging and tinkering.\n Copyright 2020 VintagePC <https://github.com/vintagepc/> with contributions from leptun, wavexx and 3d-gussner.",' ',version::VERSION_STRING); // NOLINT
	SwitchArg argWait("w","wait","Wait after the printer (and any PTYs) are set up but before starting execution.", cmd);
	MultiSwitchArg argSpam("v","verbose","Increases verbosity of the output, where supported.",cmd);
	ValueArg<int> argVCDRate("","tracerate", "Sets the logging frequency of the VCD trace (default 100uS)",false, 100,"integer",cmd);
	MultiArg<string> argVCD("t","trace","Enables VCD traces for the specified categories or IRQs. use '-t ?' to get a printout of available traces",false,"string",cmd);
	SwitchArg argTest("","test","Run it test mode (no graphics, don't auto-exit.", cmd);
	ValueArg<string> argSD("","sdimage","Use the given SD card .img file instead of the default", false ,"", "filename.img", cmd);
	SwitchArg argSerial("s","serial","Connect a printer's serial port to a PTY instead of printing its output to the console.", cmd);
	SwitchArg argScriptHelp("","scripthelp", "Prints the available scripting commands for the current printer/context",cmd, false);
	ValueArg<string> argScript("","script","Execute the given script. Use --scripthelp for syntax.", false ,"", "filename.txt", cmd);
	SwitchArg argNoHacks("n","no-hacks","Disable any special hackery that might have been implemented for a board to run its manufacturer firmware, e.g. if you want to run stock marlin and have issues. Effects depend on the board and firmware.",cmd);
	SwitchArg argMute("m","mute","Tell a printer to mute any audio it may produce.", cmd);
	SwitchArg argLoad("l","loadfw","Directs the printer to load the default firmware file. (-f implies -l) If neither -l or -f are provided, the printer executes solely from its persisted flash.", cmd);
	SwitchArg argKeyHelp("k","keys","Prints the list of available keyboard controls",cmd,false);
	std::vector<string> vstrSizes = FatImage::GetSizes();
	ValuesConstraint<string> vcSizes(vstrSizes);
	ValueArg<string> argImgSize("","image-size","Specify a size for a new SD image. You must specify an image with --sdimage",false,"256M",&vcSizes,cmd);
	SwitchArg argGDB("","gdb","Enable SimAVR's GDB support",cmd);
	std::vector<string> vstrGfx = {"none","lite","fancy", "bear"};
	ValuesConstraint<string> vcGfxAllowed(vstrGfx);
	ValueArg<string> argGfx("g","graphics","Whether to enable fancy (advanced) or lite (minimal advanced) visuals. If not specified, only the basic 2D visuals are shown.",false,"lite",&vcGfxAllowed, cmd);
	ValueArg<string> argFW("f","firmware","hex/afx/elf Firmware file to load (default MK3S.afx)",false,"MK3S.afx","filename", cmd);
	std::vector<string> vstrExts = PrintVisualType::GetOpts();
	ValuesConstraint<string> vcPrintOpts(vstrExts);
	ValueArg<string> argExtrusion("","extrusion","Set Print visual type. HR options create a LOT of triangles, do not use for large prints!",false, "Line", &vcPrintOpts, cmd);
	MultiSwitchArg argDebug("d","debug","Increases debugging output, where supported.", cmd);
	SwitchArg argColourE("", "colour-extrusion", "Colours extrusion by width (for advanced step/extrusion debugging.", cmd, false);
	SwitchArg argBootloader("b","bootloader","Run bootloader on first start instead of going straight to the firmware.",cmd);
	SwitchArg argMD("","markdown","Used to auto-generate the items in refs/ as markdown",cmd);

	std::vector<string> vstrPrinters = PrinterFactory::GetModels();
	ValuesConstraint<string> vcAllowed(vstrPrinters);

	TCLAP::UnlabeledValueArg<string> argModel("printer","Model name of the printer to run",false,"Prusa_MK3S",&vcAllowed, cmd);

	if (version::IS_DEV_VERSION)
	{
		std::cout << "***************************************" << '\n';
		std::cout << "* " << std::setw(35) << version::VERSION_STRING << " *" << '\n';
		std::cout << "* This is a DEV version. Features may *" << '\n';
		std::cout << "* behave unexpectedly, or not at all. *" << '\n';
		std::cout << "***************************************" << '\n';
	}

	cmd.parse(argc,argv);

	// Make new image.
	if (argImgSize.isSet())
	{
		if(!argSD.isSet())
		{
			std::cerr << "Cannot create an SD image without a filename." << '\n';
			exit(1);
		}
		if (FatImage::MakeFatImage(argSD.getValue(), argImgSize.getValue()))
		{
			std::cout << "Wrote " << argSD.getValue() << ". You can now use mcopy to copy gcode files into the image." << '\n';
		}
		return 0;
	}
	bool bNoGraphics = argGfx.isSet() && (argGfx.getValue()=="none");

	m_bTestMode = (argModel.getValue()=="Test_Printer") | argTest.isSet();

	Config::Get().SetExtrusionMode(PrintVisualType::GetNameToType().at(argExtrusion.getValue()));
	Config::Get().SetColourE(argColourE.isSet());

	TelemetryHost::GetHost().SetCategories(argVCD.getValue());

	ScriptHost::Init();

	std::string strFW;
	if (!argLoad.isSet() && !argFW.isSet())
	{
		strFW = ""; // No firmware and no load directive.
	}
	else
	{
		strFW = argFW.getValue();
	}

	void *pRawPrinter = PrinterFactory::CreatePrinter(argModel.getValue(),pBoard,printer,argBootloader.isSet(),argNoHacks.isSet(),argSerial.isSet(), argSD.getValue() ,
		strFW,argSpam.getValue(), argGDB.isSet(), argVCDRate.getValue(),"stk500boot_v2_mega2560.hex"); // this line is the CreateBoard() args.

	pBoard->SetPrimary(true); // This is the primary board, responsible for scripting/dispatch. Blocks contention from sub-boards, e.g. MMU.

	if (!bNoGraphics)
	{
		glutInit(&argc, argv);		/* initialize GLUT system */

		std::pair<int,int> winSize = printer->GetWindowSize();
		int pixsize = 4;
		iWinW = winSize.first * pixsize;
		iWinH = winSize.second * pixsize;
		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#ifndef TEST_MODE
		glutSetOption(GLUT_MULTISAMPLE,2);
		//glutInitContextVersion(1,0);
		glutInitDisplayMode(US(GLUT_RGB) | US(GLUT_DOUBLE) | US(GLUT_MULTISAMPLE));
#else
		glutInitDisplayMode(US(GLUT_RGB) | US(GLUT_DOUBLE));
#endif
		glutInitWindowSize(iWinW, iWinH);		/* width=400pixels height=500pixels */
		window = glutCreateWindow(GetBaseTitle().c_str());	/* create window */

		glewInit();
		std::cout << "GL_VERSION   : " << glGetString(GL_VERSION) << '\n';
		std::cout << "GL_VENDOR    : " << glGetString(GL_VENDOR) << '\n';
		std::cout << "GL_RENDERER  : " << glGetString(GL_RENDERER) << '\n';
		std::cout << "GLEW_VERSION : " << glewGetString(GLEW_VERSION) << '\n';
		//cout << "GLSL VERSION : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
#if !defined(__APPLE__)
		glDebugMessageCallback( GLErrorCB, nullptr );
		if (argSpam.getValue()<1)
		{
			glDebugMessageControl(GL_DONT_CARE,
						GL_DONT_CARE,
						GL_DEBUG_SEVERITY_NOTIFICATION,
						0, nullptr, GL_FALSE);
		}
		glEnable(GL_DEBUG_OUTPUT);
#endif
		initGL();

		if (argGfx.isSet())
		{
			printer->SetVisualType(argGfx.getValue());
		}


	}

	if (argKeyHelp.isSet())
	{
		KeyController::GetController().PrintKeys(argMD.isSet());
		exit(0);
	}

	if (argVCD.isSet() && argVCD.getValue().at(0)=="?")
	{
		TelemetryHost::GetHost().PrintTelemetry(argMD.isSet());
		exit(0);
	}

	if (argScriptHelp.isSet())
	{
		ScriptHost::PrintScriptHelp(argMD.isSet());
		return 0;
	}

	if (argScript.isSet())
	{
		if (!ScriptHost::Setup(argScript.getValue(),pBoard->GetAVR()->frequency))
		{
			return 1; // validate will have printed error info.
		}
	}
	else
	{
		ScriptHost::Setup("",pBoard->GetAVR()->frequency);
	}

	if (!bNoGraphics)
	{
		ScriptHost::CreateRootMenu(window);
	}

	// This is a little lazy, I know. Figure it out once we have non-einsy printers.
	if (argMute.isSet())
	{
		KeyController::GetController().OnKeyPressed('m');
	}

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot
	if (argWait.isSet())
	{
		std::cout << "Paused - press any key to resume execution" << '\n';
		getchar();
	}

	pBoard->StartAVR();

	if (!bNoGraphics)
	{
		glutMainLoop();
	}

	std::cout << "Waiting for board to finish..." << '\n';
	if (!m_bTestMode)
	{
		pBoard->SetQuitFlag();
	}
	pBoard->WaitForFinish();

	PrinterFactory::DestroyPrinterByName(argModel.getValue(), pRawPrinter);

	std::cout << "Done" << '\n';
	if (argScript.isSet())
	{
		return static_cast<int>(ScriptHost::GetState());
	}

}
