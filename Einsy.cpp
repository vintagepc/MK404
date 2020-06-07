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

int main(int argc, char *argv[])
{
	std::string strModel = "Prusa_MK3S";
	std::string strFW;
	bool bWait = false, bMMU = false, bLoadFW = false;
	int iArgStart = 1;
	if (strncmp(argv[1],"-",1)!=0)
	{
		iArgStart++;
		// Got a printer model.
		if (!strcmp(argv[1],"Prusa_MK3SMMU2"))
		{
			Prusa_MK3S *p = new Prusa_MK3SMMU2();
			p->CreateBoard();
			pBoard = p;
			printer = p;
		}
		else if (!strcmp(argv[1],"Prusa_MK3S"))
		{
			Prusa_MK3S *p = new Prusa_MK3S();
			p->CreateBoard();
			pBoard = p;
			printer = p;
		}
	}
	else
	{
		Prusa_MK3S *p = new Prusa_MK3S();
		p->CreateBoard();
		pBoard = p;
		printer = p;
	}

	int debug = 0;
	uint8_t chrLogSerial = ' ';
	int verbose = 1;
	for (int i = iArgStart; i < argc; i++) {
		if (!strcmp(argv[i] + strlen(argv[i]) - 4, ".hex"))
			strFW.copy(argv[i],strlen(argv[i]));
		else if (!strcmp(argv[i], "-d"))
			debug++;
		else if (!strcmp(argv[i], "-v"))
			verbose++;
		else if (!strcmp(argv[i], "-b"))
			pBoard->SetStartBootloader();
		else if (!strcmp(argv[i], "-w"))
			bWait = true;
		else if (!strcmp(argv[i], "-l"))
			bLoadFW = true;
		else if (!strcmp(argv[i], "-m"))
			bMMU = true;
		else if (!strcmp(argv[i], "--lite"))
			printer->SetVisualType(Printer::VisualType::SIMPLE);
		else if (!strcmp(argv[i], "--fancy"))
			printer->SetVisualType(Printer::VisualType::ADVANCED);
		else if (!strcmp(argv[i], "-S0"))
			printer->SetConnectSerial(true);
		else if (!strncmp(argv[i], "-lg",3))
			chrLogSerial = argv[i][3];
		else {
			fprintf(stderr, "%s: invalid argument %s\n", argv[0], argv[i]);
			exit(1);
		}
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
	if (bWait)
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
