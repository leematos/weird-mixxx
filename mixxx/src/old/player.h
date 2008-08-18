/***************************************************************************
                          player.h  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYER_H
#define PLAYER_H

#include "configobject.h"
#include <qstring.h>
#include "engineobject.h"
#include <qstringlist.h>

#define MAX_AUDIODEVICES 2

class EngineMaster;
class ControlObject;

class Player : public EngineObject
{
public:
    Player(ConfigObject<ConfigValue> *pConfig);
    virtual ~Player();
    /** Initialize the API. Returns true on success. No other methods in a Player
      * class can be called before this method returns true. Called by the proxy
      * class. */
    virtual bool initialize() = 0;
    /** Returns the current buffer size (total latency) used by the playback system */
    //static short int getBufferSize();
    /** Set EngineMaster object */
    static void setMaster(EngineMaster *pMaster);
    /** Open devices according to config database, and start audio stream.
      * Returns true on success. */
    virtual bool open();
    /** Close devices, and stop audio stream */
    virtual void close() = 0;
    /** Store default configuration in config database */
    virtual void setDefaults() = 0;
    /** Return list of interfaces */
    virtual QStringList getInterfaces() = 0;
    /** Return list of input interfaces */
    virtual QStringList getInputInterfaces() = 0;    
    /** Return list of sample rates */
    virtual QStringList getSampleRates() = 0;
    /** Return name of sound api */
    virtual QString getSoundApiName() = 0;

protected:
    /** Prepares one buffer of sound by calling the engine */
    CSAMPLE *prepareBuffer(int iBufferSize);

    /** Pointer to config database */
    ConfigObject<ConfigValue> *m_pConfig;
    /** Pointer to EngineMaster object */
    static EngineMaster *m_pMaster;
    CSAMPLE *m_pBuffer;
    /** Current buffer size in use. */
    static short int m_iBufferSize;
    /** Current number of channels in use */
    static short int m_iChannels[MAX_AUDIODEVICES];
    /** Pointer to object holding sample rate */
    ControlObject *m_pControlObjectSampleRate;
    /** Pointer to object holding latency */
    ControlObject *m_pControlObjectLatency;
};

#endif