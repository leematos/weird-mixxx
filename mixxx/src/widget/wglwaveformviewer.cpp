
#include <QGLWidget>
#include <QDebug>
#include <QDomNode>
#include <QEvent>
#include <QDragEnterEvent>
#include <QUrl>
#include <QPainter>


#include "mixxx.h"
#include "reader.h"
#include "trackinfoobject.h"
#include "wglwaveformviewer.h"
#include "waveform/waveformrenderer.h"

extern MixxxApp *mixxx_instance;

WGLWaveformViewer::WGLWaveformViewer(const char *group, QWidget * pParent, const QGLWidget * pShareWidget, Qt::WFlags f) : QGLWidget(QGLFormat(QGL::SampleBuffers), pParent, pShareWidget)
{

    m_pWaveformRenderer = new WaveformRenderer(group);

    m_pGroup = group;
    
    setAcceptDrops(true);

    installEventFilter(this);

    // Start a timer based on our desired FPS
    // TODO Eventually make this user-configurable.
    int desired_fps = 40;
    int update_interval = 1000 / desired_fps;
    m_iTimerID = startTimer(update_interval);
    
    m_painting = false;
}

bool WGLWaveformViewer::directRendering()
{
    return format().directRendering();
}


WGLWaveformViewer::~WGLWaveformViewer() {
    // Stop the timer we started
    killTimer(m_iTimerID);

    if(m_pWaveformRenderer) {
        delete m_pWaveformRenderer;
        m_pWaveformRenderer = NULL;
    }
}

void WGLWaveformViewer::setup(QDomNode node) {

    // Acquire position
    QString pos = WWidget::selectNodeQString(node, "Pos");
    int sep = pos.indexOf(",");
    int x = pos.left(sep).toInt();
    int y = pos.mid(sep+1).toInt();
    
    move(x,y);

    // Acquire size
    QString size = WWidget::selectNodeQString(node, "Size");
    sep = size.indexOf(",");
    x = size.left(sep).toInt();
    y = size.mid(sep+1).toInt();

    setFixedSize(x,y);

    m_pWaveformRenderer->resize(x,y);

    m_pWaveformRenderer->setup(node);
}

void WGLWaveformViewer::paintEvent(QPaintEvent *event) {
    QPainter painter;
    painter.begin(this);
    
    painter.setRenderHint(QPainter::Antialiasing);
    /*
    double absPlaypos;
    double bufferPlaypos;
    
    EngineBuffer *ourBuffer;

    if(QString(m_pGroup) == QString("[Channel1]")) {
        ourBuffer = mixxx_instance->buffer1;
    } else {
        ourBuffer = mixxx_instance->buffer2;
    }
    
    ourBuffer->lockPlayposVars();
    absPlaypos = ourBuffer->getAbsPlaypos();
    bufferPlaypos = ourBuffer->getBufferPlaypos();
    ourBuffer->unlockPlayposVars();
    m_pWaveformRenderer->slotUpdatePlayPos(absPlaypos/ourBuffer->getReader()->getFileLength());
    */
    m_pWaveformRenderer->draw(&painter, event);
    
    painter.end();
    m_painting = false;
    // QPainter goes out of scope and is destructed
}

void WGLWaveformViewer::timerEvent(QTimerEvent *qte) {
    //m_paintMutex.lock();
    if(!m_painting) {
        m_painting = true;
               
        // The docs say update is better than repaint.
        update();
        //updateGL();
    }
    //m_paintMutex.unlock();
}


void WGLWaveformViewer::initializeGL() {
    //qDebug() << "QGL initializeGL";

    /*
    // setup backface culling so CCW polygons are facing out
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // enables zbuffer
    glEnable(GL_DEPTH_TEST);

    // enable alpha blending
    glEnable(GL_BLEND);
    */
    

}

void WGLWaveformViewer::resizeGL(int w, int h) {
    //qDebug() << "QGL resizeGL " << w << " : " << h;

    //m_pWaveformRenderer->resize(w,h);    
}

void WGLWaveformViewer::paintGL() {
    //qDebug() << "QGL paintGL";

    //m_pWaveformRenderer->glDraw();
    //m_painting = false;
}



/** SLOTS **/

void WGLWaveformViewer::slotNewTrack(TrackInfoObject *pTrack) {
    qDebug() << "WGLWaveformViewer() << slotNewTrack() ";

    if(m_pWaveformRenderer) {
        m_pWaveformRenderer->newTrack(pTrack);
    }
    
}

void WGLWaveformViewer::setValue(double) {
    // unused, stops a bad connect from happening
}



bool WGLWaveformViewer::eventFilter(QObject *o, QEvent *e) {
    if(e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *m = (QMouseEvent*)e;
        m_iMouseStart= -1;
        if(m->button() == Qt::LeftButton) {
            // The left button went down, so store the start position
            m_iMouseStart = m->x();
            emit(valueChangedLeftDown(64));
        }
    } else if(e->type() == QEvent::MouseMove) {
        // Only send signals for mouse moving if the left button is pressed
        if(m_iMouseStart != -1) {
            QMouseEvent *m = (QMouseEvent*)e;

            // start at the middle of 0-127, and emit values based on
            // how far the mouse has travelled horizontally
            double v = 64 + (double)(m->x()-m_iMouseStart)/10;
            // clamp to 0-127
            if(v<0)
                v = 0;
            else if(v > 127)
                v = 127;
            emit(valueChangedLeftDown(v));

        }
    } else if(e->type() == QEvent::MouseButtonRelease) {
        emit(valueChangedLeftDown(64));
    } else {
        return QObject::eventFilter(o,e);
    }
    return true;
}

/** DRAG AND DROP **/


void WGLWaveformViewer::dragEnterEvent(QDragEnterEvent * event)
{
    // Accept the enter event if the thing is a filepath.
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void WGLWaveformViewer::dropEvent(QDropEvent * event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls(event->mimeData()->urls());
        QUrl url = urls.first();
        QString name = url.toLocalFile();

        event->accept();
        emit(trackDropped(name));
    } else {
        event->ignore();
    }
}