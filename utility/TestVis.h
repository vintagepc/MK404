
#include "GLObj.h"
#include <tiny_obj_loader.h>
#include <GL/glew.h>
#include "BasePeripheral.h"
#include "HD44780GL.h"

class TestVis: public BasePeripheral
{
    public:
        #define IRQPAIRS _IRQ(X_IN,"<x.in") _IRQ(Y_IN,"<y.in") _IRQ(Z_IN,"<z.in") _IRQ(SHEET_IN,"<sheet.in") _IRQ(E_IN, "<e.in") _IRQ(SD_IN,"<SD.in")
        #include "IRQHelper.h"

        TestVis();
        void Init(avr_t *avr);
        void Draw();

        void TwistKnob(bool bDir);

        void SetLCD(HD44780GL* pLCD){m_pLCD = pLCD;}

        void MouseCB(int button, int state, int x, int y);
        void MotionCB(int x, int y);
        void SetWindow(int iWin) { m_iWindow = iWin;};


    private:
       
        GLObj m_Extruder = GLObj("../assets/X_Axis.obj");
        GLObj m_Z = GLObj("../assets/Z_AXIS.obj");
        GLObj m_Y = GLObj("../assets/Y_AXIS.obj");
        GLObj m_Sheet = GLObj("../assets/Sheet.obj");
        GLObj m_Base = GLObj("../assets/Stationary.obj");
        GLObj m_SDCard = GLObj("../assets/SDCard.obj");
        GLObj m_Knob = GLObj("../assets/LCD-knobR2.obj");
        GLObj m_EVis = GLObj("../assets/Triangles.obj");

        HD44780GL *m_pLCD = nullptr;

        bool m_bLite = false; // Lite graphics

        void OnXChanged(avr_irq_t *irq, uint32_t value);
        void OnYChanged(avr_irq_t *irq, uint32_t value);
        void OnZChanged(avr_irq_t *irq, uint32_t value);
        void OnEChanged(avr_irq_t *irq, uint32_t value);
        void OnSheetChanged(avr_irq_t *irq, uint32_t value);
        void OnSDChanged(avr_irq_t *irq, uint32_t value);

        float m_fXCorr = 0.044, m_fXPos = 0.010;
        float m_fYCorr = 0.141, m_fYPos = 0.010;
        float m_fZCorr = 0.206, m_fZPos = 0.010;
        float m_fEPos = 0;

        float m_bDirty = false;

        int height = 800, width = 800, m_iWindow = 0;

        int m_iKnobPos = 0;

        bool bLoaded = false;
        double prevMouseX, prevMouseY;
        bool mouseLeftPressed;
        bool mouseMiddlePressed;
        bool mouseRightPressed;
        float curr_quat[4];
        float prev_quat[4];
        float eye[3], lookat[3], up[3], maxExtent;
};