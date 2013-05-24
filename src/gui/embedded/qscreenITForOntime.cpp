/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qscreenITForOntime.h"


#include <rttarget.h>

#define VBE_INFO_ADDR ((const void *)0x00000C00)
#define VBE_MODE_ADDR ((const void *)0x00000E00)

static int qphysHeight = 500;
static int qphysWidth = 600;

void qsetDisplayGeometry(QSize s)
{
    qphysHeight = s.height();
    qphysWidth = s.width();
}


struct VBE_INFO
{
    char sig[4];
    short version;
    char * oemstr;
    DWORD cap;
    WORD *vm;
    WORD mem;
    WORD sr;
    char * vname;
    char * vpname;
    DWORD vprev;
    WORD res;
    WORD odata;
};

struct VBE_MODE
{
    WORD ma;
    BYTE waa;//DWORD waa;
    BYTE wba;	//DWORD wba;
    WORD wgran;
    WORD wsize;
    WORD waseg;
    WORD wbseg;
    DWORD wfunc;
    WORD bpsl;
    WORD xres;
    WORD yres;
    BYTE xcsize;//	DWORD xcsize;
    BYTE ycsize;//DWORD ycsize;
    BYTE nofp;//WORD nofp;
    BYTE bpp;
    DWORD nbank;
    DWORD memmodel;
    DWORD bsize;
    DWORD nofip;
    DWORD res1;

    DWORD rms;
    DWORD rfp;
    DWORD gms;
    DWORD gfp;
    DWORD bms;
    DWORD bfp;
    DWORD resms;
    DWORD resfp;
    DWORD dcm;
    DWORD pbptr;
    DWORD res2;
    WORD res3;

    WORD lbpsc;
    DWORD bnofip;
    DWORD lnofip;
    DWORD lrms;
    DWORD lrfp;
    DWORD lgms;
    DWORD lbfp;
    DWORD llrms;
    DWORD mpc;
    DWORD rec4;
};

bool QITONTIMEScreen::connect(const QString &displaySpec)
{
    VBE_INFO *vbe_i = (VBE_INFO*)RTMapPhysMem(VBE_INFO_ADDR, sizeof(VBE_INFO), RT_PG_USERREADWRITE);
    if(strnicmp(vbe_i->sig, "vesa", 4)) return false;

    VBE_MODE *vbe_m = (VBE_MODE*)RTMapPhysMem(VBE_MODE_ADDR, sizeof(VBE_MODE), RT_PG_USERREADWRITE);
    dw = w = vbe_m->xres;
    dh = h = vbe_m->yres;
    d = vbe_m->bpp;

    char *sw = getenv("Q_DISPLAY_SIZE");

    if(!(sw && sscanf(sw, "%i %i", &physWidth, &physHeight) >= 2))
    {
        physHeight = 300;
        physWidth = 250;
    }

    lstep = w*(d>>3);

    mapsize = size = w*h*(d>>3);

    data = (uchar*)RTMapPhysMem((const void *)vbe_m->nofip, size, RT_PG_USERREADWRITE);

    QScreenCursor::initSoftwareCursor();
    return true;
}
