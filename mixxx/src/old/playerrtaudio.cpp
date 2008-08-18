/***************************************************************************
                          playerrtaudio.cpp  -  description
                             -------------------
    begin                : Thu May 20 2004
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

#include "playerrtaudio.h"
#include "controlobject.h"

/** Maximum frame size used with RtAudio. Used to determine no of buffers
 * when setting latency */
const int kiMaxFrameSize = 64;


PlayerRtAudio::PlayerRtAudio(ConfigObject<ConfigValue> * config) : Player(config)
{
    m_pRtAudio = 0;
    m_bInit = false;

    m_devId = -1;
    m_iNumberOfBuffers = 2;
    m_iChannels = -1;
    m_iMasterLeftCh = -1;
    m_iMasterRigthCh = -1;
    m_iHeadLeftCh = -1;
    m_iHeadRightCh = -1;
}

PlayerRtAudio::~PlayerRtAudio()
{
    if (m_devId>=1)
    {
        try {
            m_pRtAudio->stopStream();
            m_pRtAudio->closeStream();
        }
        catch (RtError &error)
        {
            error.printMessage();
        }
    }

    if (m_bInit)
        delete m_pRtAudio;
}

bool PlayerRtAudio::initialize()
{
    try {
        m_pRtAudio = new RtAudio();
    }
    catch (RtError &error)
    {
        error.printMessage();
        m_pRtAudio = 0;
        m_bInit = false;
    }
    m_bInit = true;

    return m_bInit;
}

bool PlayerRtAudio::open()
{
    Player::open();

    // Find out which device to open. Select the first one listed as either Master Left,
    // Master Right, Head Left, Head Right. If other devices are requested for opening
    // than the one selected here, set them to "None" in the config database
    int id = -1;
    int temp = -1;
    QString name;
    /** Maximum number of channels needed */
    int iChannelMax = -1;

    m_iMasterLeftCh = -1;
    m_iMasterRigthCh = -1;
    m_iHeadLeftCh = -1;
    m_iHeadRightCh = -1;

    // Master left
    name = m_pConfig->getValueString(ConfigKey("[Soundcard]","DeviceMasterLeft"));
    temp = getDeviceID(name);
    if (temp>=0)
    {
        if (getChannelNo(name)>=0)
        {
            id = temp;
            iChannelMax = getChannelNo(name);
            m_iMasterLeftCh = getChannelNo(name)-1;
        }
    }

    // Master right
    name = m_pConfig->getValueString(ConfigKey("[Soundcard]","DeviceMasterRight"));
    temp = getDeviceID(name);
    if (getChannelNo(name)>=0 && ((id==-1 && temp>=0) || (temp!=-1 && id==temp)))
    {
        id = temp;
        iChannelMax = math_max(iChannelMax, getChannelNo(name));
        m_iMasterRigthCh = getChannelNo(name)-1;
    }

    // Head left
    name = m_pConfig->getValueString(ConfigKey("[Soundcard]","DeviceHeadLeft"));
    temp = getDeviceID(name);
    if (getChannelNo(name)>=0 && ((id==-1 && temp>=0) || (temp!=-1 && id==temp)))
    {
        id = temp;
        iChannelMax = math_max(iChannelMax, getChannelNo(name));
        m_iHeadLeftCh = getChannelNo(name)-1;
    }

    // Head right
    name = m_pConfig->getValueString(ConfigKey("[Soundcard]","DeviceHeadRight"));
    temp = getDeviceID(name);
    if (getChannelNo(name)>=0 && ((id==-1 && temp>=0) || (temp!=-1 && id==temp)))
    {
        id = temp;
        iChannelMax = math_max(iChannelMax, getChannelNo(name));
        m_iHeadRightCh = getChannelNo(name)-1;
    }

    // Check if any of the devices in the config database needs to be set to "None"
    if (m_iMasterLeftCh<0)
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterLeft"),ConfigValue("None"));
    if (m_iMasterRigthCh<0)
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterRight"),ConfigValue("None"));
    if (m_iHeadLeftCh<0)
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceHeadLeft"),ConfigValue("None"));
    if (m_iHeadRightCh<0)
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceHeadRight"),ConfigValue("None"));

    // Number of channels to open
    int iChannels = iChannelMax;

    // Sample rate
    int iSrate = m_pConfig->getValueString(ConfigKey("[Soundcard]","Samplerate")).toInt();

    // Get latency in msec
    int iLatencyMSec = m_pConfig->getValueString(ConfigKey("[Soundcard]","Latency")).toInt();

    // Latency in samples
    int iLatencySamples = (int)((float)(iSrate*iChannels)/1000.f*(float)iLatencyMSec);

    // Apply simple rule to determine number of buffers
    if (iLatencySamples/kiMaxFrameSize<2)
        m_iNumberOfBuffers = 2;
    else
        m_iNumberOfBuffers = iLatencySamples/kiMaxFrameSize;

    // Frame size...
    int iFramesPerBuffer = iLatencySamples/m_iNumberOfBuffers;

    qDebug() << "RtAudio: id " << id << ", sr " << iSrate << ", ch " << iChannels << ", bufsize " << iFramesPerBuffer << ", bufno " << m_iNumberOfBuffers;

    if (id<1)
    {
        m_devId = -1;
        return false;
    }

    // Start playback
    try
    {
        // Update SRATE and Latency ControlObjects
        m_pControlObjectSampleRate->queueFromThread((double)iSrate);
        m_pControlObjectLatency->queueFromThread((double)iLatencyMSec);

        // Open stream
        m_pRtAudio->openStream(id, iChannels, 0, 0, RTAUDIO_FLOAT32, iSrate, &iFramesPerBuffer, m_iNumberOfBuffers);

        // Set callback function
        m_pRtAudio->setStreamCallback(&rtCallback, (void *)this);

        m_iChannels = iChannels;
        m_devId = id;

        // Start playback
        m_pRtAudio->startStream();
    }
    catch (RtError &error)
    {
        error.printMessage();

        m_devId = -1;
        m_iChannels = -1;

        return false;
    }

    return true;
}

void PlayerRtAudio::close()
{
    m_iChannels = 0;
    m_iMasterLeftCh = -1;
    m_iMasterRigthCh = -1;
    m_iHeadLeftCh = -1;
    m_iHeadRightCh = -1;

    if (m_devId>0)
    {
        qDebug() << "close";
        try
        {
            m_pRtAudio->stopStream();
            m_pRtAudio->closeStream();
        }
        catch (RtError &error)
        {
            error.printMessage();
        }
    }
    m_devId = -1;
}

void PlayerRtAudio::setDefaults()
{
    // Get list of interfaces
    QStringList interfaces = getInterfaces();

    // Set first interfaces to master left
    QStringList::iterator it = interfaces.begin();
    if (it!=interfaces.end())
    {
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterLeft"),ConfigValue((*it)));
    }
    else
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterLeft"),ConfigValue("None"));

    // Set second interface to master right
    ++it;
    if (it!=interfaces.end())
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterRight"),ConfigValue((*it)));
    else
        m_pConfig->set(ConfigKey("[Soundcard]","DeviceMasterRight"),ConfigValue("None"));

    // Set head left and right to none
    m_pConfig->set(ConfigKey("[Soundcard]","DeviceHeadLeft"),ConfigValue("None"));
    m_pConfig->set(ConfigKey("[Soundcard]","DeviceHeadRight"),ConfigValue("None"));

    // Set default sample rate
    QStringList srates = getSampleRates();
    it = srates.begin();
    while (it!=srates.end())
    {
        m_pConfig->set(ConfigKey("[Soundcard]","Samplerate"),ConfigValue((*it)));

        if ((*it).toInt()>=44100)
            break;

        ++it;
    }

    // Set currently used latency in config database
    int msec = (int)(1000.*(2.*1024.)/(2.*(float)(*it).toInt()));
    m_pConfig->set(ConfigKey("[Soundcard]","Latency"), ConfigValue(msec));
}

QStringList PlayerRtAudio::getInterfaces()
{
    QStringList result;

    int no = m_pRtAudio->getDeviceCount();
    RtAudioDeviceInfo info;
    for (int i=1; i<=no; i++)
    {
        bool bGotInfo = false;
        try {
            info = m_pRtAudio->getDeviceInfo(i);
            bGotInfo = true;
        }
        catch (RtError &error)
        {
            error.printMessage();
        }

        // Add the device if it is an output device:
        if (bGotInfo && info.outputChannels > 0)
        {
            qDebug() << "name " << info.name.c_str();
            for (int j=1; j<=info.outputChannels; ++j)
                result.append(QString("%1 (channel %2)").arg(info.name.c_str()).arg(j));
        }
    }

    return result;
}

QStringList PlayerRtAudio::getSampleRates()
{
    // Returns the list of supported sample rates of the currently opened device.
    // If no device is open, return the list of sample rates supported by the
    // default device
    int id = m_devId;
    if (id<1)
        id = 1;

    RtAudioDeviceInfo info;
    try {
        info = m_pRtAudio->getDeviceInfo(id);
    }
    catch (RtError &error)
    {
        error.printMessage();
        return QStringList();
    }

    // Sample rates
    QValueList<int> srlist;

    if (info.sampleRates.size() > 0)
    {
        for (unsigned int j=0; j<info.sampleRates.size(); ++j)
            srlist.append(info.sampleRates[j]);
    }

    // Sort list
#ifndef QT3_SUPPORT
    qHeapSort(srlist);
#endif

    // Convert srlist to stringlist
    QStringList result;
    for (int i=0; i<srlist.count(); ++i)
        result.append(QString("%1").arg((*srlist.at(i))));

    return result;
}

QString PlayerRtAudio::getSoundApi()
{
#ifdef __WIN__
    return QString("DirectSound");
#endif
#ifdef __LINUX__
    return QString("ALSA");
#endif
}

int PlayerRtAudio::getDeviceID(QString name)
{
    int no = m_pRtAudio->getDeviceCount();
    for (int i=1; i<=no; i++)
    {
        RtAudioDeviceInfo info;
        try {
            info = m_pRtAudio->getDeviceInfo(i);
        }
        catch (RtError &error)
        {
            error.printMessage();
            return -1;
        }

        // Add the device if it is an output device:
        if (info.outputChannels > 0)
        {
            for (int j=1; j<=info.outputChannels; ++j)
                if (QString("%1 (channel %2)").arg(info.name.c_str()).arg(j) == name)
                    return i;
        }
    }
    return -1;
}

int PlayerRtAudio::getChannelNo(QString name)
{
    int no = m_pRtAudio->getDeviceCount();
    for (int i=1; i<=no; i++)
    {
        RtAudioDeviceInfo info;
        try {
            info = m_pRtAudio->getDeviceInfo(i);
        }
        catch (RtError &error)
        {
            error.printMessage();
            return -1;
        }

        // Add the device if it is an output device:
        if (info.outputChannels > 0)
        {
            for (int j=1; j<=info.outputChannels; ++j)
                if (QString("%1 (channel %2)").arg(info.name.c_str()).arg(j) == name)
                    return j;
        }
    }
    return -1;
}

int PlayerRtAudio::callbackProcess(int iBufferSize, float * out)
{
    //m_iBufferSize = iBufferSize*m_iNumberOfBuffers;

    float * tmp = prepareBuffer(iBufferSize);
    float * output = out;
    int i;

    // Reset sample for each open channel
    for (i=0; i<iBufferSize*m_iChannels; i++)
        output[i] = 0.;

    // Copy to output buffer
    for (i=0; i<iBufferSize; i++)
    {
        if (m_iMasterLeftCh>=0) output[m_iMasterLeftCh]  += tmp[(i*4)  ]/32768.;
        if (m_iMasterRigthCh>=0) output[m_iMasterRigthCh] += tmp[(i*4)+1]/32768.;
        if (m_iHeadLeftCh>=0) output[m_iHeadLeftCh]    += tmp[(i*4)+2]/32768.;
        if (m_iHeadRightCh>=0) output[m_iHeadRightCh]   += tmp[(i*4)+3]/32768.;

        for (int j=0; j<m_iChannels; ++j)
            *output++;
    }

    return 0;
}

/* -------- ------------------------------------------------------
   Purpose: Wrapper function to call processing loop function,
            implemented as a method in a class. Used in RtAudio.
   Input:   .
   Output:  -
   -------- ------------------------------------------------------ */
int rtCallback(char * outputBuffer, int framesPerBuffer, void * pPlayer)
{
    return ((PlayerRtAudio *)pPlayer)->callbackProcess(framesPerBuffer, (float *)outputBuffer);
}