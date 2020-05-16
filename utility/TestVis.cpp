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

}

void TestVis::Init(avr_t *avr)
{
  _Init(avr,this);
  RegisterNotify(X_IN,MAKE_C_CALLBACK(TestVis,OnXChanged),this);
  RegisterNotify(Y_IN,MAKE_C_CALLBACK(TestVis,OnYChanged),this);
  RegisterNotify(Z_IN,MAKE_C_CALLBACK(TestVis,OnZChanged),this);
  RegisterNotify(E_IN,MAKE_C_CALLBACK(TestVis,OnEChanged),this);
  RegisterNotify(SHEET_IN, MAKE_C_CALLBACK(TestVis, OnSheetChanged), this);
  RegisterNotify(EFAN_IN, MAKE_C_CALLBACK(TestVis, OnEFanChanged), this);
  m_bDirty = true;
}

void TestVis::TwistKnob(bool bDir)
{
  if (bDir)
    m_iKnobPos = (m_iKnobPos+18)%360;
  else
    m_iKnobPos = (m_iKnobPos + 342)%360;
}

void TestVis::OnXChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fXPos =  fPos[0]/1000.f;
  m_bDirty = true;
}

void TestVis::OnSheetChanged(avr_irq_t *irq, uint32_t value)
{
  m_Sheet.SetAllVisible(value>0);
  m_bDirty = true;
}

void TestVis::OnEFanChanged(avr_irq_t *irq, uint32_t value)
{
  m_bFanOn = (value>0);
  m_bDirty = true;
}

void TestVis::OnSDChanged(avr_irq_t *irq, uint32_t value)
{
  m_SDCard.SetAllVisible(value^1);
  m_bDirty = true;
}

void TestVis::OnYChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fYPos =  fPos[0]/1000.f;
  m_bDirty = true;
}

void TestVis::OnEChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fEPos =  fPos[0]/1000.f;
  m_bDirty = true;
}

void TestVis::OnZChanged(avr_irq_t *irq, uint32_t value)
{
  float* fPos = (float*)(&value); // both 32 bits, just mangle it for sending over the wire.
  m_fZPos =  fPos[0]/1000.f;
  m_bDirty = true;
}

void TestVis::Draw()
{
    //if (!m_bDirty)
    //    return;
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
    float fAmb[] = {.1,.1,.1,1};
    float fCol2[] = {1,1,1,1};
    float fSpec[] = {.4,.4,.4,.5};
    float fDiff[] = {2.5,2.5,2.5,1};

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
    float fMM2M = 1.f/1000.f;
    float fExtent = m_Base.GetScaleFactor();
    // Fit to -1, 1
    glScalef(1.0f / fExtent, 1.0f / fExtent, 1.0f / fExtent);
    float fTransform[3];
    m_Base.GetCenteringTransform(fTransform);
    // Centerize object.
    glTranslatef (fTransform[0], fTransform[1], fTransform[2]);

    glPushMatrix();   
      glTranslatef(0,-m_fZCorr + (m_fZPos),0);
      m_Z.Draw();
      glPushMatrix();
        glTranslatef(-m_fXCorr + (m_fXPos),0,0);
        m_Extruder.Draw();

        glScalef(fMM2M,fMM2M,fMM2M);
        m_EStd.Draw();
        glPushMatrix();
          if (m_bFanOn)
            m_iFanPos = (m_iFanPos + 23)%360;
          m_EFan.GetCenteringTransform(fTransform);
          glTranslatef (-fTransform[0], -fTransform[1], -fTransform[2]);
          glRotatef((float)m_iFanPos,1,0,0);
          glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
          m_EFan.Draw();
        glPopMatrix();
        glPushMatrix();  
          m_EVis.GetCenteringTransform(fTransform);
          fTransform[1] +=1.5f;
          glTranslatef (-fTransform[0] , -fTransform[1], -fTransform[2]);
          glRotatef((-36.f/28.f)*3.f*(m_fEPos*1000.f),0,0,1);
          glTranslatef (fTransform[0], fTransform[1], fTransform[2]);
          m_EVis.Draw();
        glPopMatrix();
      glPopMatrix();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(0,0, -m_fYCorr + (m_fYPos));
      m_Y.Draw();
      glTranslatef(0.025,0.083,0.431);
      m_Sheet.Draw();
      glTranslatef(0,0.01,0);
      float fLED[4] = {1,0,0,1};
      glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE | GL_SPECULAR,fNone);
      glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,fLED);
      glBegin(GL_QUADS);
        glVertex3f(0,0,0);
        glVertex3f(0,0,0.1);
        glVertex3f(0.1,0,0.1);
        glVertex3f(0.1,0,0);
      glEnd();
    glPopMatrix();
    m_Base.Draw();
    glPushMatrix();
      glTranslatef(0.215,0.051,0.501);
      glRotatef(-45.f,1,0,0);
      glPushMatrix();  
        glRotatef((float)m_iKnobPos,0,0,1);
        m_Knob.Draw();
      glPopMatrix();
    glPopMatrix();
    if (m_pLCD)
    {
      glPushMatrix();
        glTranslatef(0.101,0.0549,0.4925);
        glRotatef(-45.f,1,0,0);
        glPushMatrix();  
        float fScale = (4.f*0.076f)/500.f; // Disp is 76mm wide, lcd is drawn 500 wide at 4x scale
          glScalef(fScale,fScale,fScale);
          glScalef(1.0,-1.0f,-0.1f);
          glPushMatrix();
            m_pLCD->Draw(0x02c5fb00, 0x8d7ff8ff, 0xFFFFFFff, 0x00000055, true);
          glPopMatrix();
        glPopMatrix();
      glPopMatrix();
    }
    glPushMatrix();
      glScalef(fMM2M,fMM2M,fMM2M);
      m_SDCard.Draw();
    glPopMatrix();

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
