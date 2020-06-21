/*
	Einsy.cpp - MK404 printer simulator for the Prusa i3 MK2/3 range.
	Dubbed MK404 as a tribute to their web 404 page. ;-)

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


#include <unistd.h>
#include <sys/stat.h>
#include <type_traits>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <signal.h>

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#endif
#include <pthread.h>

extern "C" {
	#include "sim_vcd_file.h"
}

#include <tclap/CmdLine.h>

#include "PrinterFactory.h"
#include "parts/Board.h"
#include "Printer.h"

#include "FatImage.h"

avr_vcd_t vcd_file;

uint8_t gbPrintPC = 0;

int window;

Printer *printer = nullptr;
Boards::Board *pBoard = nullptr;

bool m_bStopping = false;

// Exit cleanly on ^C
void OnSigINT(int iSig) {
	if (!m_bStopping)
	{
		printf("Caught SIGINT... stopping...\n");
		m_bStopping = true;
		if (pBoard)
			pBoard->SetQuitFlag();
	}
	else
	{
		printf("OK, OK! I get the message!\n");
		exit(2);
	}
}

void displayCB(void)		/* function called whenever redisplay needed */
{
	glutSetWindow(window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix

	printer->Draw();

	glutSwapBuffers();
}

void keyCB(unsigned char key, int x, int y)	/* called on key press */
{
	printer->OnKeyPress(key,x,y);
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
	glutTimerFunc(50, timerCB, i^1);
	displayCB();
}

int initGL(int w, int h)
{
	// Set up projection matrix
	glMatrixMode(GL_PROJECTION); // Select projection matrix
	glLoadIdentity(); // Start with an identity matrix
	glOrtho(0, w, 0, h, 0, 10);
	glScalef(1,-1,1);
	glTranslatef(0, -1 * h, 0);


	glutDisplayFunc(displayCB);		/* set window's display callback */
	glutKeyboardFunc(keyCB);		/* set window's key callback */
	glutMouseFunc(MouseCB);
	glutMotionFunc(MotionCB);
	glutTimerFunc(1000, timerCB, 0);

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return 1;
}

void * glutThread(void* p)
{
    glutMainLoop();
    return NULL;
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
	SwitchArg argLoad("l","loadfw","Directs the printer to load the default firmware file. (-f implies -l) If neither -l or -f are provided, the printer executes solely from its persisted flash.");
	cmd.add(argLoad);
	vector<string> vstrSizes = FatImage::GetSizes();
	ValuesConstraint<string> vcSizes(vstrSizes);
	ValueArg<string> argImgSize("","image-size","Specify a size for a new SD image. You must specify an image with --sdimage",false,"256M",&vcSizes);
	cmd.add(argImgSize);
	vector<string> vstrGfx = {"none","lite","fancy"};
	ValuesConstraint<string> vcGfxAllowed(vstrGfx);
	ValueArg<string> argGfx("g","graphics","Whether to enable fancy (advanced) or lite (minimal advanced) visuals. If not specified, only the basic 2D visuals are shown.",false,"lite",&vcGfxAllowed);
	cmd.add(argGfx);
	ValueArg<string> argFW("f","firmware","hex/afx/elf Firmware file to load (default MK3S.afx)",false,"MK3S.afx","filename");
	cmd.add(argFW);
	MultiSwitchArg argDebug("d","debug","Increases debugging output, where supported.");
	cmd.add(argDebug);
	SwitchArg argBootloader("b","bootloader","Run bootloader on first start instead of going straight to the firmware.");
	cmd.add(argBootloader);

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

	std::string strFW;
	if (!argLoad.isSet() && !argFW.isSet())
		strFW = ""; // No firmware and no load directive.
	else
		strFW = argFW.getValue();

	void *pRawPrinter = PrinterFactory::CreatePrinter(argModel.getValue(),pBoard,printer,argBootloader.isSet(),argNoHacks.isSet(),argSerial.isSet(), argSD.getValue() ,strFW,argSpam.getValue());

	if (!bNoGraphics)
	{
	glutInit(&argc, argv);		/* initialize GLUT system */

	std::pair<int,int> winSize = printer->GetWindowSize();
	int w = winSize.first;
	int h = winSize.second;
	int pixsize = 4;

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(w * pixsize, h * pixsize);		/* width=400pixels height=500pixels */
	window = glutCreateWindow("Prusa i3 MK404 (PRINTER NOT FOUND) ('q' quits)");	/* create window */

	initGL(w * pixsize, h * pixsize);

	if (argGfx.isSet())
	{
		if (vstrGfx[0].compare(argGfx.getValue())==0)
			printer->SetVisualType(Printer::VisualType::SIMPLE);
		else
			printer->SetVisualType(Printer::VisualType::ADVANCED);

	}
	};
	if (argScriptHelp.isSet())
	{
		ScriptHost::Init("",0);
		ScriptHost::PrintScriptHelp();
		return 0;
	}

	if (argScript.isSet())
	{
		if (!ScriptHost::Init(argScript.getValue(), pBoard->GetAVR()->frequency))
			return 1; // validate will have printed error info.
	}

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot
	if (argWait.isSet())
	{
		printf("Paused - press any key to resume execution\n");
		getchar();
	}

    pthread_t run;
	if (!bNoGraphics)
  	  pthread_create(&run, NULL, glutThread, NULL);

	pBoard->StartAVR();

	pBoard->WaitForFinish();

	if (!bNoGraphics)
	{
		glutLeaveMainLoop();
		pthread_cancel(run); // Kill the GL thread.
	}

	PrinterFactory::DestroyPrinterByName(argModel.getValue(), pRawPrinter);

	printf("Done\n");
	if (argScript.isSet())
		return (int)ScriptHost::GetState();

}
