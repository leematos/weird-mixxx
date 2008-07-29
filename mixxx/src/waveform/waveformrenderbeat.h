
#ifndef WAVEFORMRENDERBEAT_H
#define WAVEFORMRENDERBEAT_H

#include <QObject>
#include <QColor>
#include <QVector>

class QDomNode;
class QPainter;
class QPaintEvent;


class ControlObjectThreadMain;
class WaveformRenderer;
class TrackInfoObject;
class SoundSourceProxy;

class WaveformRenderBeat : public QObject {
    Q_OBJECT
public:
    void resize(int w, int h);
    void setup(QDomNode node);
    WaveformRenderBeat(const char *group, WaveformRenderer *parent);
    void draw(QPainter *pPainter, QPaintEvent *event, QVector<float> *buffer, double playPos);
    void newTrack(TrackInfoObject *pTrack);

public slots:
    void slotUpdateBpm(double bpm);
    void slotUpdateBeatFirst(double beatfirst);
    void slotUpdateTrackSamples(double samples);
private:
    WaveformRenderer *m_pParent;
    ControlObjectThreadMain *m_pBpm;
    ControlObjectThreadMain *m_pBeatFirst;
    ControlObjectThreadMain *m_pTrackSamples;
    TrackInfoObject *m_pTrack;
    int m_iWidth, m_iHeight;
    double m_dBpm;
    double m_dBeatFirst;
    QColor colorMarks;
    double m_dSamplesPerPixel;
    double m_dSamplesPerDownsample;
    double m_dBeatLength;
    int m_iNumSamples;
    int m_iSampleRate;
};

#endif