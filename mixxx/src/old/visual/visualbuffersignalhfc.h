//
// C++ Interface: visualbuffersignalhfc
//
// Description:
//
//
// Author: Tue Haste Andersen <haste@diku.dk>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef VISUALBUFFERSIGNALHFC_H
#define VISUALBUFFERSIGNALHFC_H

#include "visualbuffersignal.h"

/**
@author Tue Haste Andersen
*/
class VisualBufferSignalHFC : public VisualBufferSignal
{
public:
    VisualBufferSignalHFC(ReaderExtract *pReaderExtract, EngineBuffer *pEngineBuffer, const char *group);
    ~VisualBufferSignalHFC();

    void update(int iPos, int iLen, long int, int);
    void draw(GLfloat *p, int iLen, float);
};

#endif