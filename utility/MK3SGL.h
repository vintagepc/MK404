
#include "GLObj.h"
#include <tiny_obj_loader.h>
#include <GL/glew.h>
#include "BasePeripheral.h"
#include "HD44780GL.h"

class MK3SGL: public BasePeripheral
{
    public:
        #define IRQPAIRS    _IRQ(X_IN,"<x.in") _IRQ(Y_IN,"<y.in") _IRQ(Z_IN,"<z.in") \
                            _IRQ(SHEET_IN,"<sheet.in") _IRQ(E_IN, "<e.in") _IRQ(SD_IN,"<SD.in") _IRQ(EFAN_IN,"<EFAN.in") \
                            _IRQ(BED_IN,"<bed.in") _IRQ(PINDA_IN,"<pinda.in") _IRQ(PFAN_IN,"<PFAN.in") _IRQ(SEL_IN,"<Sel.in") \
                            _IRQ(IDL_IN,"<idler.in") _IRQ(MMU_LEDS_IN,"<mmuleds.in")
        #include "IRQHelper.h"

        MK3SGL(bool bLite);
        void Init(avr_t *avr);
        void Draw();

        void TwistKnob(bool bDir);

        void SetLCD(HD44780GL* pLCD){m_pLCD = pLCD;}

        void SetMMU(bool bMMU) { m_bMMU = bMMU;}

        void SetLite(bool bLite) { m_bLite = bLite;}

        void MouseCB(int button, int state, int x, int y);
        void MotionCB(int x, int y);
        void SetWindow(int iWin) { m_iWindow = iWin;};


    private:
       
        GLObj m_Extruder = GLObj("assets/X_AXIS.obj");
        GLObj m_Z = GLObj("assets/Z_AXIS.obj");
        GLObj m_Y = GLObj("assets/Y_AXIS.obj");
        GLObj m_Sheet = GLObj("assets/SSSheet.obj");
        GLObj m_Base = GLObj("assets/Stationary.obj");
        GLObj m_SDCard = GLObj("assets/SDCard.obj");
        GLObj m_Knob = GLObj("assets/LCD-knobR2.obj");
        GLObj m_EVis = GLObj("assets/Triangles.obj");
        GLObj m_EMMU = GLObj("assets/E_MMU.obj");
        GLObj m_EStd = GLObj("assets/E_STD.obj");
        GLObj m_EFan = GLObj("assets/E_Fan.obj");
        GLObj m_EPFan = GLObj("assets/Print-fan_rotor.obj");
        GLObj m_MMUBase = GLObj("assets/MMU_stationary.obj");
        GLObj m_MMUSel = GLObj("assets/MMU_Selector.obj");
        GLObj m_MMUIdl = GLObj("assets/Idler_moving.obj");

        HD44780GL *m_pLCD = nullptr;

        bool m_bLite = false; // Lite graphics

        void DrawMMU();
        void DrawLED(float r, float g, float b);
        void OnXChanged(avr_irq_t *irq, uint32_t value);
        void OnYChanged(avr_irq_t *irq, uint32_t value);
        void OnZChanged(avr_irq_t *irq, uint32_t value);
        void OnEChanged(avr_irq_t *irq, uint32_t value);
        void OnSelChanged(avr_irq_t *irq, uint32_t value);
        void OnIdlChanged(avr_irq_t *irq, uint32_t value);
        void OnMMULedsChanged(avr_irq_t *irq, uint32_t value);
        void OnSheetChanged(avr_irq_t *irq, uint32_t value);
        void OnSDChanged(avr_irq_t *irq, uint32_t value);
        void OnEFanChanged(avr_irq_t *irq, uint32_t value);
        void OnPFanChanged(avr_irq_t *irq, uint32_t value);
        void OnBedChanged(avr_irq_t *irq, uint32_t value);
        void OnPINDAChanged(avr_irq_t *irq, uint32_t value);

        float m_fXCorr = 0.044, m_fXPos = 0.010;
        float m_fYCorr = 0.141, m_fYPos = 0.010;
        float m_fZCorr = 0.206, m_fZPos = 0.010;
        float m_fEPos = 0;
        float m_fSelCorr = 0.025f, m_fSelPos = 0.0f;
        // This is going to be in degrees rotation instead of mm
        float m_fIdlCorr = 20.00f, m_fIdlPos = 0.0f;
        
        int m_iKnobPos = 0, m_iFanPos = 0, m_iPFanPos = 0;

        bool m_bDirty = false, m_bFanOn = false, m_bMMU = false, m_bBedOn = false, m_bPINDAOn = false;
        bool m_bPFanOn = false;

        int height = 800, width = 800, m_iWindow = 0;


        double prevMouseX, prevMouseY;
        bool mouseLeftPressed;
        bool mouseMiddlePressed;
        bool mouseRightPressed;
        float curr_quat[4];
        float prev_quat[4];
        float eye[3], lookat[3], up[3], maxExtent;
};