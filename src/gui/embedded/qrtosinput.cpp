#include "qrtosinput.h"
#include <windows.h>
#include <qthread.h>
#include <rtk32.h>
#include <rttarget.h>
#include "qapplication.h"
#include "private/qapplication_p.h"
#include "../kernel/qeventdispatcher_qws_p.h"
#include "qsemaphore.h"


static rtiMouseHandler mouseAction = 0;
static rtiKeyHandler keyUp = 0, keyDown = 0;
static QMutex *rti = 0;
static HANDLE winIO = 0;

DWORD WINAPI rtiProcessEvents(void *);

inline void rtiInit()
{
    if(!rti)
    {
        rti = new QMutex();
        winIO = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ,
                       0,OPEN_EXISTING,0,0);
        CreateThread(0, 0, rtiProcessEvents, 0, 0, 0);       
    }
}

void QRTINPUTSetMouseHandler(rtiMouseHandler mouse)
{
    rtiInit();
    QMutexLocker locker(rti);
    mouseAction = mouse;
}

void QRTINPUTSetKeyHandler(rtiKeyHandler down, rtiKeyHandler up)
{
    rtiInit();
    QMutexLocker locker(rti);

    keyUp = up;
    keyDown = down;
}


DWORD WINAPI rtiProcessEvents(void *)
{
    INPUT_RECORD e;
    DWORD res;
    while(1)
    if(ReadConsoleInputA(winIO, &e, 1, &res))
    {
        QMutexLocker locker(rti);
        if(e.EventType == MOUSE_EVENT)
        {
                int buttons = (e.Event.MouseEvent.dwButtonState &
                                    FROM_LEFT_1ST_BUTTON_PRESSED ? Qt::LeftButton: 0)|
                                  (e.Event.MouseEvent.dwButtonState &
                                  RIGHTMOST_BUTTON_PRESSED ? Qt::RightButton:0);
                if(mouseAction)
                   mouseAction(QPoint(e.Event.MouseEvent.dwMousePosition.X,
                    e.Event.MouseEvent.dwMousePosition.Y), (Qt::MouseButton)buttons);

        }else if(e.EventType == KEY_EVENT)
        {
            int st = e.Event.KeyEvent.dwControlKeyState;
            int state = (st & (SHIFT_PRESSED) ? Qt::ShiftModifier : 0)|
                    (st & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED) ? Qt::ControlModifier : 0)|
                    (st & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED) ? Qt::AltModifier : 0);

            int kc = e.Event.KeyEvent.wVirtualKeyCode;

            switch(kc)
            {
                case VK_ESCAPE: kc = Qt::Key_Escape; break;
                case VK_F1: kc = Qt::Key_F1; break;
                case VK_F2: kc = Qt::Key_F2; break;
                case VK_F3: kc = Qt::Key_F3; break;
                case VK_F4: kc = Qt::Key_F4; break;
                case VK_F5: kc = Qt::Key_F5; break;
                case VK_F6: kc = Qt::Key_F6; break;
                case VK_F7: kc = Qt::Key_F7; break;
                case VK_F8: kc = Qt::Key_F8; break;
                case VK_F9: kc = Qt::Key_F9; break;
                case VK_F10: kc = Qt::Key_F10; break;
                case VK_F11: kc = Qt::Key_F11; break;
                case VK_F12: kc = Qt::Key_F12; break;
                case VK_PRINT: kc = Qt::Key_Print; break;
                case VK_SCROLL: kc = Qt::Key_ScrollLock; break;
                case VK_PAUSE: kc = Qt::Key_Pause; break;

                case VK_RETURN: kc = Qt::Key_Return; break;
                case VK_SPACE: kc = Qt::Key_Space; break;
                case VK_TAB: kc = Qt::Key_Tab; break;
                case VK_BACK: kc = Qt::Key_BackSpace; break;
                case VK_CAPITAL: kc = Qt::Key_CapsLock; break;
                case VK_RMENU:
                case VK_LMENU: kc = Qt::Key_Menu; break;

                case VK_INSERT: kc = Qt::Key_Insert; break;
                case VK_DELETE: kc = Qt::Key_Delete; break;
                case VK_HOME: kc = Qt::Key_Home; break;
                case VK_END: kc = Qt::Key_End; break;
                case VK_PRIOR: kc = Qt::Key_PageUp; break;
                case VK_NEXT: kc = Qt::Key_PageDown; break;
                case VK_UP: kc = Qt::Key_Up; break;
                case VK_DOWN: kc = Qt::Key_Down; break;
                case VK_LEFT: kc = Qt::Key_Left; break;
                case VK_RIGHT: kc = Qt::Key_Right; break;

                case VK_NUMLOCK: kc = Qt::Key_NumLock; break;

                case VK_RCONTROL:
                case VK_LCONTROL:
                case VK_CONTROL: kc = Qt::Key_Control; break;
                case VK_LSHIFT:
                case VK_RSHIFT:
                case VK_SHIFT: kc = Qt::Key_Shift; break;
            };

            if(e.Event.KeyEvent.bKeyDown)
            {
                if(keyDown)
                    keyDown(e.Event.KeyEvent.uChar.UnicodeChar, kc, (Qt::KeyboardModifier)state);
            }
            else if(keyUp) keyUp(e.Event.KeyEvent.uChar.UnicodeChar, kc, (Qt::KeyboardModifier)state);
        }

        QApplicationPrivate *pa = QApplicationPrivate::instance();
        if(pa && pa->eventDispatcher)
            pa->eventDispatcher->wakeUp();
	}
}

