/**

  A license and other info goes here!

 */

#include <QDebug>
#include <QDomNode>
#include <QImage>
#include <QObject>

#include <time.h>

#include "waveformrenderer.h"
#include "waveformrenderbeat.h"
#include "waveformrendermark.h"
#include "trackinfoobject.h"
#include "soundsourceproxy.h"
#include "controlobjectthreadmain.h"
#include "controlobject.h"
#include "wwidget.h"
#include "wskincolor.h"

#define DEFAULT_SUBPIXELS_PER_PIXEL 4
#define DEFAULT_PIXELS_PER_SECOND 100

WaveformRenderer::WaveformRenderer(const char* group) :
m_iWidth(0),
m_iHeight(0),
m_iNumSamples(0),
bgColor(0,0,0),
signalColor(255,255,255),
colorMarker(255,255,255),
colorBeat(255,255,255),
colorCue(255,255,255),
m_lines(0),
m_iSubpixelsPerPixel(DEFAULT_SUBPIXELS_PER_PIXEL),
m_iPixelsPerSecond(DEFAULT_PIXELS_PER_SECOND),
m_pImage(),
m_dPlayPos(0),
m_dPlayPosOld(-1),
m_iPlayPosTime(-1),
m_iPlayPosTimeOld(-1),
m_pTrack(NULL),
m_backgroundPixmap(),
m_bRepaintBackground(true),
QObject()
{
    m_pPlayPos = new ControlObjectThreadMain(ControlObject::getControl(ConfigKey(group,"visual_playposition")));
    if(m_pPlayPos != NULL)
        connect(m_pPlayPos, SIGNAL(valueChanged(double)), this, SLOT(slotUpdatePlayPos(double)));

    m_pRenderBeat = new WaveformRenderBeat(group, this);
    m_pRenderCue = new WaveformRenderMark(group, ConfigKey(group, "cue_point"), this);

    m_pCOVisualResample = new ControlObject(ConfigKey(group, "VisualResample"));
}


WaveformRenderer::~WaveformRenderer() {
    if(m_pCOVisualResample)
        delete m_pCOVisualResample;
    m_pCOVisualResample = NULL;

    if(m_pPlayPos)
        delete m_pPlayPos;
    m_pPlayPos = NULL;

    if(m_pRenderBeat)
        delete m_pRenderBeat;
    m_pRenderBeat = NULL;

    if(m_pRenderCue)
        delete m_pRenderCue;
    m_pRenderCue = NULL;

}

void WaveformRenderer::slotUpdatePlayPos(double v) {
    m_iPlayPosTimeOld = m_iPlayPosTime;
    m_dPlayPosOld = m_dPlayPos;    
    m_dPlayPos = v;
    m_iPlayPosTime = clock();
}

void WaveformRenderer::resize(int w, int h) {
    m_iWidth = w;
    m_iHeight = h;
    m_lines.resize(w*m_iSubpixelsPerPixel);

    setupControlObjects();

    // Need to repaint the background if we've been resized.
    m_backgroundPixmap.resize(w,h);
    m_bRepaintBackground = true;

    // Notify children that we've been resized
    m_pRenderBeat->resize(w,h);
    m_pRenderCue->resize(w,h);
}

void WaveformRenderer::setupControlObjects() {

    // the resample rate is the number of samples that correspond to one downsample

    // This set of restrictions provides for a downsampling setup like this:

    // Let a sample be a sample in the original song.
    // Let a downsample be a sample in the downsampled buffer
    // Let a pixel be a pixel on the screen.
    
    // W samples -> X downsamples -> Y pixels

    // We start with the restriction that we desire 1 second of
    // 'raw' information to be contained within Z real pixels of screen space.

    // 1) 1 second / z pixels = f samples / z pixels  = (f/z) samples per pixel

    // The size of the buffer we interact with is the number of downsamples

    // The ratio of samples to downsamples is N : 1
    // The ratio of downsamples to pixels is M : 1

    // Therefore the ratio of samples to pixels is MN : 1

    // Or in other words, we have MN samples per pixel

    // 2) MN samples / pixel

    // We combine 1 and 2 into one constraint:

    // (f/z) = mn, or  f = m * n * z
    
    // REQUIRE : M * N * Z = F
    // M : DOWNSAMPLES PER PIXEL
    // N : SAMPLES PER DOWNSAMPLE
    // F : SAMPLE RATE OF SONG
    // Z : THE USER SEES 1 SECOND OF DATA IN Z PIXELS

    // Solving for N, the number of samples in our downsample buffer,
    // we get : N = F / (M*Z)

    // We don't know F, so we're going to transmit M*Z

    double m = m_iSubpixelsPerPixel; // M DOWNSAMPLES PER PIXEL
    double z = m_iPixelsPerSecond; // Z PIXELS REPRESENTS 1 SECOND OF DATA
        
    m_pCOVisualResample->set(m*z);

    qDebug() << "WaveformRenderer::setupControlObjects - VisualResample: " << m*z;

}

void WaveformRenderer::setup(QDomNode node) {

    bgColor.setNamedColor(WWidget::selectNodeQString(node, "BgColor"));
    bgColor = WSkinColor::getCorrectColor(bgColor);

    qDebug() << "Got bgColor " << bgColor;
    
    signalColor.setNamedColor(WWidget::selectNodeQString(node, "SignalColor"));
    signalColor = WSkinColor::getCorrectColor(signalColor);

    qDebug() << "Got signalColor " << signalColor;

    colorMarker.setNamedColor(WWidget::selectNodeQString(node, "MarkerColor"));
    colorMarker = WSkinColor::getCorrectColor(colorMarker);

    colorBeat.setNamedColor(WWidget::selectNodeQString(node, "BeatColor"));
    colorBeat = WSkinColor::getCorrectColor(colorBeat);

    colorCue.setNamedColor(WWidget::selectNodeQString(node, "CueColor"));
    colorCue = WSkinColor::getCorrectColor(colorCue);

    m_pRenderBeat->setup(node);

    m_pRenderCue->setup(node);
}


void WaveformRenderer::precomputePixmap() {
    if(m_pSampleBuffer == NULL || m_iNumSamples == 0 || !m_pImage.isNull())
        return;

    qDebug() << "Generating a image!";

    int monoSamples = (m_iNumSamples >> 3);
    qDebug() << monoSamples << " samples for qimage";
    QImage qi(monoSamples, m_iHeight, QImage::Format_RGB32);
    
    QPainter paint;
    paint.begin(&qi);

    paint.fillRect(qi.rect(), QBrush(QColor(255,0,0)));//bgColor));//QColor(0,0,0)));
    paint.setPen(QColor(0,255,0));//signalColor);//QColor(0,255,0));
    qDebug() << "height " << m_iHeight;
    paint.translate(0,m_iHeight/2);
    paint.scale(1.0,-1.0);
    paint.drawLine(QLine(0,0,monoSamples,0));
    //for (int i=0;i<100;i++) {
        //paint.drawLine(QLine(i,0,i,m_iHeight/2));
        //paint.drawLine(QLine(i,0,i,m_iHeight/2));
    //}
    
    for(int i=0;i<monoSamples;i++) {
        //SAMPLE sampl = (*m_pSampleBuffer)[i*2];
        //SAMPLE sampr = (*m_pSampleBuffer)[i*2+1];

        //paint.drawLine(QLine(i,-5, i, 0));
        paint.drawLine(QLine(i,2, i, 80));
        //paint.drawLine(QLine(i,-sampr,i,sampl));
    }
    paint.end();
    qDebug() << "done with image";
    qi.save("/home/rryan/foo.bmp", "BMP", 100);
    m_pImage = qi;

    
    return;

    /*
    qDebug() << "Generating a pixmap!";

    // Now generate a pixmap of this
    QPixmap *pm = new QPixmap(m_iNumSamples/2, m_iHeight);

    if(pm->isNull()) {
        qDebug() << "Built a null pixmap, WTF!";
    } else {
        qDebug() << " Build a pixmap " << pm->size();
    }
    
    QPainter paint;
    paint.begin(pm);

    qDebug() << "Wave Precomp: BG: " << bgColor << " FG:" << signalColor;
    paint.fillRect(pm->rect(), QBrush(bgColor));//QColor(0,0,0)));
    paint.setPen(signalColor);//QColor(0,255,0));

    paint.translate(0,m_iHeight/2);
    paint.scale(1.0,-1.0);
    //paint.drawLine(QLine(0,0,resultSamples/2,0));
    
    for(int i=0;i<m_iNumSamples/2;i++) {
        SAMPLE sampl = (*m_pSampleBuffer)[i*2];
        SAMPLE sampr = (*m_pSampleBuffer)[i*2+1];

        //paint.drawLine(QLine(i,-15, i, 15));
        paint.drawLine(QLine(i,-sampr,i,sampl));
    }
    paint.end();
    
    */
}

bool WaveformRenderer::fetchWaveformFromTrack() {
    
    if(!m_pTrack)
        return false;

    QVector<float> *buffer = m_pTrack->getVisualWaveform();

    if(buffer == NULL)
        return false;

    m_pSampleBuffer = buffer;
    m_iNumSamples = buffer->size();

    return true;
}
    


void WaveformRenderer::drawSignalLines(QPainter *pPainter,double playpos) {
    
    if(m_pSampleBuffer == NULL) {
        return;
    }

    int iCurPos = 0;
    if(m_dPlayPos != -1) {
        iCurPos = (int)(playpos*m_iNumSamples);
    }
        
    if((iCurPos % 2) != 0)
        iCurPos--;

    pPainter->save();

    int subpixelWidth = m_iWidth * m_iSubpixelsPerPixel;

    pPainter->scale(1.0/float(m_iSubpixelsPerPixel),m_iHeight*0.40);
    int halfw = subpixelWidth/2;
    for(int i=0;i<subpixelWidth;i++) {
        // Start at curPos minus half the waveform viewer
        int thisIndex = iCurPos+2*(i-halfw);
        if(thisIndex >= 0 && (thisIndex+1) < m_iNumSamples) {
            float sampl = (*m_pSampleBuffer)[thisIndex];
            float sampr = (*m_pSampleBuffer)[thisIndex+1];
            m_lines[i] = QLineF(i,-sampr,i,sampl);
        } else {
            m_lines[i] = QLineF(0,0,0,0);
        }
    }

    pPainter->drawLines(m_lines);

    pPainter->restore();
}

void WaveformRenderer::drawSignalPixmap(QPainter *pPainter) {


    //if(m_pImage == NULL)
    //return;
    if(m_pImage.isNull())
        return;
    
    //double dCurPos = m_pPlayPos->get();
    int iCurPos = (int)(m_dPlayPos*m_pImage.width());
        
    int halfw = m_iWidth/2;
    int halfh = m_iHeight/2;

    int totalHeight = m_pImage.height();
    int totalWidth = m_pImage.width();
    int width = m_iWidth;
    int height = m_iHeight;
    // widths and heights of the two rects should be the same:
    // m_iWidth - 0 = iCurPos + halfw - iCurPos + halfw = m_iWidth (if even)
    // -halfh-halfh = -halfh-halfh

    int sx=iCurPos-halfw;
    int sy=0;
    int tx=0;
    int ty=0;

    if(sx < 0) {
        sx = 0;
        width = iCurPos + halfw;
        tx = m_iWidth - width;
    } else if(sx + width >= totalWidth) {
        //width = (iCurPos - sx) + (totalWidth-iCurPos);
        width = halfw + totalWidth - iCurPos;
    }

    QRect target(tx,ty,width,height);
    QRect source(sx,sy,width,height);

    //qDebug() << "target:" << target;
    //qDebug() << "source:" << source;
    pPainter->setPen(signalColor);

    pPainter->drawImage(target, m_pImage, source);

}


void WaveformRenderer::generateBackgroundPixmap() {
    QLinearGradient linearGrad(QPointF(0,0), QPointF(0,m_iHeight));
    linearGrad.setColorAt(0.0, bgColor);
    linearGrad.setColorAt(0.5, bgColor.light(180));
    linearGrad.setColorAt(1.0, bgColor);
      
    // linearGrad.setColorAt(0.0, Qt::black);
    // linearGrad.setColorAt(0.3, bgColor);
    // linearGrad.setColorAt(0.7, bgColor);
    // linearGrad.setColorAt(1.0, Qt::black);
    QBrush brush(linearGrad);

    QPainter newPainter;
    newPainter.begin(&m_backgroundPixmap);
    newPainter.fillRect(m_backgroundPixmap.rect(), brush);
    newPainter.end();
    
    m_bRepaintBackground = false;
}


void WaveformRenderer::draw(QPainter* pPainter, QPaintEvent *pEvent) {
    double playposadjust = 0;

    if(m_iWidth == 0 || m_iHeight == 0)
        return;

    /*
    if(m_dPlayPos != -1 && m_dPlayPosOld != -1 && m_iNumSamples != 0) {
        double latency = ControlObject::getControl(ConfigKey("[Master]","latency"))->get();
        latency *= 4;
        latency *= CLOCKS_PER_SEC / 1000.0;
        
        //int latency = m_iPlayPosTime - m_iPlayPosTimeOld;
        double timeelapsed = (clock() - m_iPlayPosTime);
        double timeratio = 0;
        if(latency != 0)
            timeratio = double(timeelapsed) / double(latency);
        if(timeratio > 1.0)
            timeratio = 1.0;

        
        playposadjust = ((m_dPlayPos*m_iNumSamples) - (m_dPlayPosOld*m_iNumSamples)) * timeelapsed;
        playposadjust /= (latency*m_iNumSamples);

        //qDebug() << "ppold " << m_dPlayPosOld << " pp " << m_dPlayPos << " timeratio " << timeratio;
        //qDebug() << "timee" << timeelapsed <<  "playpoadj" << playposadjust;
    }
    */

    double playpos = m_dPlayPos + playposadjust;
    
    if(m_bRepaintBackground) {
        generateBackgroundPixmap();
    }

    // Paint the background
    pPainter->drawPixmap(m_backgroundPixmap.rect(), m_backgroundPixmap, pEvent->rect());

    pPainter->setPen(signalColor);
    
    
    if(m_pSampleBuffer == NULL) {
        fetchWaveformFromTrack();
        if(m_pSampleBuffer != NULL)
            qDebug() << "Received waveform from track";
    }

    // Translate our coordinate frame from (0,0) at top left
    // to (0,0) at left, center. All the subrenderers expect this.
    
    pPainter->translate(0.0,m_iHeight/2.0);
    // Now scale so that positive-y points up.
    pPainter->scale(1.0,-1.0);

    // Draw the center horizontal line under the signal.
    pPainter->drawLine(QLine(0,0,m_iWidth,0));
    
    drawSignalLines(pPainter,playpos);

    // Draw various markers.
    m_pRenderBeat->draw(pPainter,pEvent, m_pSampleBuffer, playpos);
    m_pRenderCue->draw(pPainter,pEvent, m_pSampleBuffer, playpos);
    
    pPainter->setPen(colorMarker);
    
    // Draw the center vertical line
    pPainter->drawLine(QLineF(m_iWidth/2.0,m_iHeight/2.0,m_iWidth/2.0,-m_iHeight/2.0));

}

void WaveformRenderer::newTrack(TrackInfoObject* pTrack) {

    m_pTrack = pTrack;
    m_pSampleBuffer = NULL;
    m_iNumSamples = 0;
    m_dPlayPos = 0;
    m_dPlayPosOld = 0;
    
    m_pRenderBeat->newTrack(pTrack);
    m_pRenderCue->newTrack(pTrack);
}

int WaveformRenderer::getPixelsPerSecond() {
    return m_iPixelsPerSecond;
}

int WaveformRenderer::getSubpixelsPerPixel() {
    return m_iSubpixelsPerPixel;
}
