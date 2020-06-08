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

#include "parts/Board.h"
#include "Printer.h"
#include "printers/Prusa_MK3S.h"
#include "printers/Prusa_MK3SMMU2.h"

avr_vcd_t vcd_file;

uint8_t gbPrintPC = 0;

int window;

Printer *printer = nullptr;
Boards::Board *pBoard = nullptr;

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
	glutPostRedisplay();
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
	CmdLine cmd("tset",' ',"0.1");
	SwitchArg argWait("w","wait","Wait after the printer (and any PTYs) are set up but before starting execution.");
	cmd.add(argWait);
	MultiSwitchArg argSpam("v","verbose","Increases verbosity of the output, where supported.");
	cmd.add(argSpam);
	SwitchArg argSerial("s","serial","Connect a printer's serial port to a PTY instead of printing its output to the console.");
	cmd.add(argSerial);
	SwitchArg argLoad("l","loadfw","Directs the printer to load the default firmware file. (-f implies -l) If neither -l or -f are provided, the printer executes solely from its persisted flash.");
	cmd.add(argLoad);
	vector<string> vstrGfx = {"lite","fancy"};
	ValuesConstraint<string> vcGfxAllowed(vstrGfx);
	ValueArg<string> argGfx("g","graphics","Whether to enable fancy (advanced) or lite (minimal advanced) visuals. If not specified, only the basic 2D visuals are shown.",false,"lite",&vcGfxAllowed);
	cmd.add(argGfx);
	ValueArg<string> argFW("f","firmware","hex/afx Firmware file to load (default MK3S.afx)",false,"MK3S.afx","filename");
	cmd.add(argFW);
	MultiSwitchArg argDebug("d","debug","Increases debugging output, where supported.");
	cmd.add(argDebug);
	SwitchArg argBootloader("b","bootloader","Run bootloader on first start instead of going straight to the firmware.");
	cmd.add(argBootloader);


	vector<string> vstrPrinters = {"Prusa_MK3S","Prusa_MK3SMMU2"};
	ValuesConstraint<string> vcAllowed(vstrPrinters);

	UnlabeledValueArg<string> argModel("printer","Model name of the printer to run",false,"Prusa_MK3S",&vcAllowed);
	cmd.add(argModel);

	cmd.parse(argc,argv);

	std::string strFW;
	if (!argLoad.isSet() && !argFW.isSet())
		strFW = ""; // No firmware and no load directive.
	else
		strFW = argFW.getValue();

	if (!argModel.isSet() || vstrPrinters[0].compare(argModel.getValue())==0)
	{
			Prusa_MK3S *p = new Prusa_MK3S(strFW, argSerial.isSet());
			if (argBootloader.isSet()) p->SetStartBootloader();
			p->CreateBoard();
			pBoard = p;
			printer = p;
	}
	else
	{
			Prusa_MK3S *p = new Prusa_MK3SMMU2(strFW, argSerial.isSet());
			if (argBootloader.isSet()) p->SetStartBootloader();
			p->CreateBoard();
			pBoard = p;
			printer = p;
	}
	if (argGfx.isSet())
	{
		if (vstrGfx[0].compare(argGfx.getValue())==0)
			printer->SetVisualType(Printer::VisualType::SIMPLE);
		else
			printer->SetVisualType(Printer::VisualType::ADVANCED);

	}

	glutInit(&argc, argv);		/* initialize GLUT system */

	std::pair<int,int> winSize = printer->GetWindowSize();
	int w = winSize.first;
	int h = winSize.second;
	int pixsize = 4;

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(w * pixsize, h * pixsize);		/* width=400pixels height=500pixels */
	window = glutCreateWindow("Prusa i3 MK404 (PRINTER NOT FOUND) ('q' quits)");	/* create window */

	initGL(w * pixsize, h * pixsize);

	// Useful for getting serial pipes/taps setup, the node exists so you can
	// start socat (or whatever) without worrying about missing a window for something you need to do at boot.
	if (argWait.isSet())
	{
		printf("Paused - press any key to resume execution\n");
		getchar();
	}

    pthread_t run;

    pthread_create(&run, NULL, glutThread, NULL);

	pBoard->StartAVR();

	pBoard->WaitForFinish();

	glutLeaveMainLoop();
	pthread_cancel(run); // Kill the GL thread.

	printf("Done\n");

}
