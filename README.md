Qt4 Framework for OnTime RTOS
---
This is a port of Qt Libraries to Win32 API Compatible RTOS for 32/64-bit x86 Embedded Systems from [OnTime](https://www.on-time.com/rtos-32.ht)

[logo]:https://www.on-time.com/images/satlarge.jpg "OnTime Logo"

Ported Modules
---
- Qt Core
- Qt Gui [1]
- Qt Network
- Qt Script
- Qt Xml
- Qt Sql


[1] - Qt Gui module uses VESA linear buffer initialized by RTOS on startup for drawing widgets. The performance of this approach might not be perfect comparing to using OpenGL graphics, but it brings a lot of fresh air into the process of building UI for embedded systems.

Please see demo project for examples.
