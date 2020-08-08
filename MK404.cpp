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


#include <GL/glew.h>                  // for glVertex2f, glEnable, glTranslatef
#include <GL/freeglut.h>          // for glutLeaveMainLoop, glutSetOption
#include <pthread.h>                  // for pthread_cancel, pthread_create
#include <signal.h>                   // for signal, SIGINT
#include <stdint.h>                   // for uint8_t
#include <stdio.h>                    // for printf, NULL, fprintf, getchar
#include <stdlib.h>                   // for exit
#include <tclap/CmdLine.h>            // for CmdLine
#include <algorithm>                  // for find
#include <memory>
#include <atomic>
#include <scoped_allocator>           // for allocator_traits<>::value_type
#include <string>                     // for string, basic_string
#include <utility>                    // for pair
#include <vector>                     // for vector
#include "FatImage.h"                 // for FatImage
#include "Printer.h"                  // for Printer, Printer::VisualType
#include "PrinterFactory.h"           // for PrinterFactory
#include "ScriptHost.h"               // for ScriptHost
#include "TelemetryHost.h"
#include "parts/Board.h"              // for Board
#include "sim_avr.h"                  // for avr_t
#include "tclap/MultiSwitchArg.h"     // for MultiSwitchArg
#include "tclap/SwitchArg.h"          // for SwitchArg
#include "tclap/UnlabeledValueArg.h"  // for UnlabeledValueArg
#include "tclap/ValueArg.h"           // for ValueArg
#include "tclap/ValuesConstraint.h"   // for ValuesConstraint
#include "Macros.h"

int window = 0;

atomic_int iWinH{0}, iWinW{0};

Printer *printer = nullptr;
Boards::Board *pBoard = nullptr;

bool m_bStopping = false;

// Exit cleanly on ^C
void OnSigINT(int iSig) {
	if (!m_bStopping)
	{
		printf("Caught SIGINT... stopping...\n");
		m_bStopping = true;
		if (printer)
			printer->OnKeyPress('q',0,0);
	}
	else
	{
		printf("OK, OK! I get the message!\n");
		exit(2);
	}
}

extern "C" {
void GLAPIENTRY
GLErrorCB( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}
}

atomic_bool bIsQuitting {false};

void displayCB(void)		/* function called whenever redisplay needed */
{
	if (bIsQuitting || pBoard->GetQuitFlag()) // Stop drawing if shutting down.
	{
		bIsQuitting = true;
		glutLeaveMainLoop();
		return;
	}
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int iW = glutGet(GLUT_WINDOW_WIDTH);
	int iH = glutGet(GLUT_WINDOW_HEIGHT);
	printer->Draw();

	if (pBoard->IsStopped() || pBoard->IsPaused())
	{
		string strState = pBoard->IsStopped() ? "Stopped" : "Paused";
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
				for (size_t i=0; i<strState.length(); i++)
					glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,strState[i]);
			glPopAttrib();
		glPopMatrix();


	}
	glutSwapBuffers();
}

void keyCB(unsigned char key, int x, int y)	/* called on key press */
{
	switch(key)
	{
		case '+':
			TelemetryHost::GetHost()->StartTrace();
			printf("Enabled VCD trace.\n");
			break;
		case '-':
			TelemetryHost::GetHost()->StopTrace();
			printf("Stopped VCD trace\n");
			break;
		default:
			printer->OnKeyPress(key,x,y);
	}
}

void MouseCB(int button, int action, int x, int y)	/* called on key press */
{
	printer->OnMousePress(button,action,x,y);
}

void MotionCB(int x, int y)
{
	printer->OnMouseMove(x,y);
}

// gl timer. if the lcd is dirty, refresh display
void timerCB(int i)
{
	if (bIsQuitting)
		return;
	glutSetWindow(window);
	if (iWinH!=glutGet(GLUT_WINDOW_HEIGHT) || iWinW != glutGet(GLUT_WINDOW_WIDTH))
		glutReshapeWindow(iWinW, iWinH);
	glutTimerFunc(50, timerCB, i^1);
	glutPostRedisplay();
}


void ResizeCB(int w, int h)
{
	std::pair<int,int> winSize = printer->GetWindowSize();
	float fWS = (float)w/(float)(winSize.first*4);
	float fHS = (float)h/(float)(winSize.second*4);
	float fScale = max(fWS,fHS);
	int iW = 4.f*(float)winSize.first*fScale;
	int iH = 4.f*(float)winSize.second*fScale;
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

int initGL(int w, int h)
{
	// Set up projection matrix
	glutDisplayFunc(displayCB);		/* set window's display callback */
	glutKeyboardFunc(keyCB);		/* set window's key callback */
	glutMouseFunc(MouseCB);
	glutMotionFunc(MotionCB);
	glutTimerFunc(1000, timerCB, 0);
	glutReshapeFunc(ResizeCB);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(.5f, 0.f, 0.f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return 1;
}

using namespace TCLAP;
using namespace std;
int main(int argc, char *argv[])
{
	signal(SIGINT, OnSigINT);

	CmdLine cmd("MK404 is an 8-bit AVR based 3D printer simulator for firmware debugging and tinkering.\n Copyright 2020 VintagePC <https://github.com/vintagepc/> with contributions from leptun, wavexx and 3d-gussner.",' ',"0.1");
	SwitchArg argWait("w","wait","Wait after the printer (and any PTYs) are set up but before starting execution.");
	cmd.add(argWait);
	MultiSwitchArg argSpam("v","verbose","Increases verbosity of the output, where supported.");
	cmd.add(argSpam);
	ValueArg<int> argVCDRate("","tracerate", "Sets the logging frequency of the VCD trace (default 100uS)",false, 100,"integer");
	cmd.add(argVCDRate);
	MultiArg<string> argVCD("t","trace","Enables VCD traces for the specified categories or IRQs. use '-t ?' to get a printout of available traces",false,"string");
	cmd.add(argVCD);
	ValueArg<string> argSD("","sdimage","Use the given SD card .img file instead of the default", false ,"", "filename.img");
	cmd.add(argSD);
	SwitchArg argSerial("s","serial","Connect a printer's serial port to a PTY instead of printing its output to the console.");
	cmd.add(argSerial);
	SwitchArg argScriptHelp("","scripthelp", "Prints the available scripting commands for the current printer/context",false);
	cmd.add(argScriptHelp);
	ValueArg<string> argScript("","script","Execute the given script. Use --scripthelp for syntax.", false ,"", "filename.txt");
	cmd.add(argScript);
	SwitchArg argNoHacks("n","no-hacks","Disable any special hackery that might have been implemented for a board to run its manufacturer firmware, e.g. if you want to run stock marlin and have issues. Effects depend on the board and firmware.");
	cmd.add(argNoHacks);
	SwitchArg argMute("m","mute","Tell a printer to mute any audio it may produce.");
	cmd.add(argMute);
	SwitchArg argLoad("l","loadfw","Directs the printer to load the default firmware file. (-f implies -l) If neither -l or -f are provided, the printer executes solely from its persisted flash.");
	cmd.add(argLoad);
	vector<string> vstrSizes = FatImage::GetSizes();
	ValuesConstraint<string> vcSizes(vstrSizes);
	ValueArg<string> argImgSize("","image-size","Specify a size for a new SD image. You must specify an image with --sdimage",false,"256M",&vcSizes);
	cmd.add(argImgSize);
	SwitchArg argGDB("","gdb","Enable SimAVR's GDB support");
	cmd.add(argGDB);
	vector<string> vstrGfx = {"none","lite","fancy", "bear"};
	ValuesConstraint<string> vcGfxAllowed(vstrGfx);
	ValueArg<string> argGfx("g","graphics","Whether to enable fancy (advanced) or lite (minimal advanced) visuals. If not specified, only the basic 2D visuals are shown.",false,"lite",&vcGfxAllowed);
	cmd.add(argGfx);
	ValueArg<string> argFW("f","firmware","hex/afx/elf Firmware file to load (default MK3S.afx)",false,"MK3S.afx","filename");
	cmd.add(argFW);
	MultiSwitchArg argDebug("d","debug","Increases debugging output, where supported.");
	cmd.add(argDebug);
	SwitchArg argBootloader("b","bootloader","Run bootloader on first start instead of going straight to the firmware.");
	cmd.add(argBootloader);
	SwitchArg argMD("","markdown","Used to auto-generate the items in refs/ as markdown");
	cmd.add(argMD);
	vector<string> vstrPrinters = PrinterFactory::GetModels();
	ValuesConstraint<string> vcAllowed(vstrPrinters);

	UnlabeledValueArg<string> argModel("printer","Model name of the printer to run",false,"Prusa_MK3S",&vcAllowed);
	cmd.add(argModel);

	cmd.parse(argc,argv);

	// Make new image.
	if (argImgSize.isSet())
	{
		if(!argSD.isSet())
		{
			fprintf(stderr,"Cannot create an SD image without a filename.\n");
			exit(1);
		}
		FatImage::MakeFatImage(argSD.getValue(), argImgSize.getValue());
		printf("Wrote %s. You can now use mcopy to copy gcode files into the image.\n",argSD.getValue().c_str());
		return 0;
	}
	bool bNoGraphics = argGfx.isSet() && (argGfx.getValue().compare("none")==0);

	TelemetryHost::GetHost()->SetCategories(argVCD.getValue());

	ScriptHost::Init();

	std::string strFW;
	if (!argLoad.isSet() && !argFW.isSet())
		strFW = ""; // No firmware and no load directive.
	else
		strFW = argFW.getValue();

	void *pRawPrinter = PrinterFactory::CreatePrinter(argModel.getValue(),pBoard,printer,argBootloader.isSet(),argNoHacks.isSet(),argSerial.isSet(), argSD.getValue() ,
		strFW,argSpam.getValue(), argGDB.isSet(), argVCDRate.getValue()); // this line is the CreateBoard() args.

	pBoard->SetPrimary(true); // This is the primary board, responsible for scripting/dispatch. Blocks contention from sub-boards, e.g. MMU.

	if (!bNoGraphics)
	{
		glutInit(&argc, argv);		/* initialize GLUT system */

		std::pair<int,int> winSize = printer->GetWindowSize();
		int pixsize = 4;
		iWinW = winSize.first * pixsize;
		iWinH = winSize.second * pixsize;
		glutSetOption(GLUT_MULTISAMPLE,2);
		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
		//glutInitContextVersion(1,0);
		glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE);
		glutInitWindowSize(iWinW, iWinH);		/* width=400pixels height=500pixels */
		window = glutCreateWindow("Prusa i3 MK404 (PRINTER NOT FOUND) ('q' quits)");	/* create window */

		glewInit();
		cout << "GL_VERSION   : " << glGetString(GL_VERSION) << endl;
		cout << "GL_VENDOR    : " << glGetString(GL_VENDOR) << endl;
		cout << "GL_RENDERER  : " << glGetString(GL_RENDERER) << endl;
		cout << "GLEW_VERSION : " << glewGetString(GLEW_VERSION) << endl;
		//cout << "GLSL VERSION : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
#if !defined(__APPLE__)
		glDebugMessageCallback( GLErrorCB, 0 );
		if (argSpam.getValue()<1)
		{
			glDebugMessageControl(GL_DONT_CARE,
						GL_DONT_CARE,
						GL_DEBUG_SEVERITY_NOTIFICATION,
						0, nullptr, GL_FALSE);
		}
		glEnable(GL_DEBUG_OUTPUT);
#endif
		initGL(iWinW, iWinH);

		if (argGfx.isSet())
			printer->SetVisualType(argGfx.getValue());


	}
	if (argVCD.isSet() && argVCD.getValue().at(0).compare("?")==0)
	{
		TelemetryHost::GetHost()->PrintTelemetry(argMD.isSet());
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
			return 1; // validate will have printed error info.
	}
	else
		ScriptHost::Setup("",pBoard->GetAVR()->frequency);

	if (!bNoGraphics)
		ScriptHost::CreateRootMenu(window);

	// This is a little lazy, I know. Figure it out once we have non-einsy printers.
	if (argMute.isSet())
		printer->OnKeyPress('m',0,0);

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot
	if (argWait.isSet())
	{
		printf("Paused - press any key to resume execution\n");
		getchar();
	}

	pBoard->StartAVR();

	if (!bNoGraphics)
		glutMainLoop();

	printf("Waiting for board to finish...\n");
	pBoard->SetQuitFlag();
	pBoard->WaitForFinish();

	PrinterFactory::DestroyPrinterByName(argModel.getValue(), pRawPrinter);

	printf("Done\n");
	if (argScript.isSet())
		return static_cast<int>(ScriptHost::GetState());

}
