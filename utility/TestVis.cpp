#include "TestVis.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <cstring>

#include <trackball.h>

#include <GL/glew.h>
#include <GL/glut.h>
TestVis::TestVis()
{
  trackball(curr_quat,0,0,0,0);
    eye[0] = 0.0f;
    eye[1] = 0.0f;
    eye[2] = 3.0f;

    lookat[0] = 0.0f;
    lookat[1] = 0.0f;
    lookat[2] = 0.0f;

    up[0] = 0.0f;
    up[1] = 1.0f;
    up[2] = 0.0f;
    if (m_bLite)
    {
        m_Base.SetAllVisible(false);
        m_Y.SetAllVisible(false);
        for (int i=1; i<4; i++)
          m_Y.SetSubobjectVisible(i); // heatbed, sheet

        m_Z.SetAllVisible(false);
        m_Extruder.SetAllVisible(false);
        m_Extruder.SetSubobjectVisible(31);
    }
    else // Disable a bunch of stuff on the Einsy board.
      //for (int i=28;i<54; i++)
        m_Base.SetSubobjectVisible(1,false);
}

void TestVis::Init(avr_t *avr)
{
  _Init(avr,this);
  RegisterNotify(X_IN,MAKE_C_CALLBACK(TestVis,OnXChanged),this);
  RegisterNotify(Y_IN,MAKE_C_CALLBACK(TestVis,OnYChanged),this);
  RegisterNotify(Z_IN,MAKE_C_CALLBACK(TestVis,OnZChanged),this);
  RegisterNotify(SHEET_IN, MAKE_C_CALLBACK(TestVis, OnSheetChanged), this);
  m_bDirty = true;
}

void TestVis::OnXChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fXPos =  fPos[0];
  m_bDirty = true;
}

void TestVis::OnSheetChanged(avr_irq_t *irq, uint32_t value)
{
  m_Y.SetSubobjectVisible(2,value>0);
  m_Y.SetSubobjectVisible(3,value>0);
  m_bDirty = true;
}

void TestVis::OnYChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fYPos =  fPos[0];
  m_bDirty = true;
}


void TestVis::OnZChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fZPos =  fPos[0];
  m_bDirty = true;
}

void TestVis::Draw()
{
    if (!m_bDirty)
        return;
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
  glLoadIdentity();
  //printf("eye: %f %f %f\n",curr_quat[0],curr_quat[1], curr_quat[2]);
    float pos[] = {2,2,2,0};
    float fpos[] = {0,1,-1,1};
    float fNone[] = {0,0,0,1};
    float fAmb[] = {0,0,0,1};
    float fCol2[] = {1,1,1,1};
    float fSpec[] = {1,1,1,1};
    float fDiff[] = {1.5,1.5,1.5,1};

    // camera & rotate

    glEnable(GL_CULL_FACE);

    glLightfv(GL_LIGHT0,GL_AMBIENT, fAmb);
    glLightfv(GL_LIGHT0,GL_SPECULAR, fSpec);
    glLightfv(GL_LIGHT0,GL_DIFFUSE, fDiff);
    glLightfv(GL_LIGHT0,GL_POSITION, pos);
    glEnable(GL_LIGHT0);

    glEnable(GL_LIGHTING); 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    GLfloat mat[4][4];
    gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0],
              up[1], up[2]);
      

    build_rotmatrix(mat, curr_quat);
    glMultMatrixf(&mat[0][0]);

    float fExtent = m_Extruder.GetScaleFactor();
    fExtent = m_Z.GetScaleFactor()>fExtent? m_Z.GetScaleFactor() : fExtent;
    fExtent = m_Y.GetScaleFactor()>fExtent? m_Y.GetScaleFactor() : fExtent;
    // Fit to -1, 1
    glScalef(1.0f / fExtent, 1.0f / fExtent, 1.0f / fExtent);
    float fTransform[3];
    m_Base.GetCenteringTransform(fTransform);
    // Centerize object.
    glTranslatef (fTransform[0], fTransform[1], fTransform[2]);

    glPushMatrix();
      glTranslatef(0,-m_fZCorr + (m_fZPos/1000),0);
      m_Z.Draw();
      glPushMatrix();
        glTranslatef(-m_fXCorr + (m_fXPos/932),0,0);
        m_Extruder.Draw();
      glPopMatrix();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(0,0, -m_fYCorr + (m_fYPos/932));
      m_Y.Draw();
    glPopMatrix();
    m_Base.Draw();

    glutSwapBuffers();
    m_bDirty = false;
}


void TestVis::MouseCB(int button, int action, int x, int y)
{
 if (button == GLUT_LEFT_BUTTON) {
    if (action == GLUT_DOWN) {
      mouseLeftPressed = true;
     // trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
    } else if (action == GLUT_UP) {
      mouseLeftPressed = false;
    }
  }
  if (button == GLUT_RIGHT_BUTTON) {
    if (action == GLUT_DOWN) {
      mouseRightPressed = true;
    } else if (action == GLUT_UP) {
      mouseRightPressed = false;
    }
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    if (action == GLUT_DOWN) {
      mouseMiddlePressed = true;
    } else if (action == GLUT_UP) {
      mouseMiddlePressed = false;
    }
  }
  if (button==3 || button==4) // wheel
  {
    if (button==3)
      eye[2] += 0.05f;
    else
      eye[2] -= 0.05f;
  }
  m_bDirty = true;
}

void TestVis::MotionCB(int x, int y)
{
      float rotScale = 1.0f;
  float transScale = 2.0f;

  if (mouseLeftPressed) {
    trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
              rotScale * (height - 2.0f * prevMouseY) / (float)height,
              rotScale * (2.0f * x - width) / (float)width,
              rotScale * (height - 2.0f * y) / (float)height);

    add_quats(prev_quat, curr_quat, curr_quat);
  } else if (mouseMiddlePressed) {
    eye[0] -= transScale * (x - prevMouseX) / (float)width;
    lookat[0] -= transScale * (x - prevMouseX) / (float)width;
    eye[1] += transScale * (y - prevMouseY) / (float)height;
    lookat[1] += transScale * (y - prevMouseY) / (float)height;
  } else if (mouseRightPressed) {
    eye[2] += transScale * (y - prevMouseY) / (float)height;
    lookat[2] += transScale * (y - prevMouseY) / (float)height;
  }

  // Update mouse point
  prevMouseX = x;
  prevMouseY = y;
  m_bDirty = true;
}
