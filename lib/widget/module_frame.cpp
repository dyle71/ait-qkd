/*
 * module_frame.cpp
 * 
 * a GUI to inspect some states of a running module
 * 
 * Author: Oliver Maurhart, <oliver.maurhart@ait.ac.at>
 *
 * Copyright (C) 2013-2016 AIT Austrian Institute of Technology
 * AIT Austrian Institute of Technology GmbH
 * Donau-City-Strasse 1 | 1220 Vienna | Austria
 * http://www.ait.ac.at
 *
 * This file is part of the AIT QKD Software Suite.
 *
 * The AIT QKD Software Suite is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * The AIT QKD Software Suite is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the AIT QKD Software Suite. 
 * If not, see <http://www.gnu.org/licenses/>.
 */


// TODO: still some buttons needed implementation (pipeline & module starters)


// ------------------------------------------------------------
// include

#include <chrono>

#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"
#endif

#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>

#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif

// ait
#include <qkd/module/module.h>
#include <qkd/utility/properties.h>
#include <qkd/widget/lcd.h>
#include <qkd/widget/led.h>
#include <qkd/widget/module_frame.h>
#include <qkd/widget/plot.h>
#include <qkd/widget/res.h>

#include "ui_module_frame.h"


using namespace qkd::widget;


// ------------------------------------------------------------
// defs


/**
 * the maximum number of items to plot
 */
#define PLOT_RANGE          400


/**
 * the plot precision in MSec (1 tick)
 */
#define TIMEOUT_MSECS       250



// ------------------------------------------------------------
// decl


/**
 * holds all current absolute values
 */
class module_values {
    

public:
    

    std::chrono::system_clock::time_point m_cTimestamp;      /**< timepoint value aquisition */
    
    struct {
        
        uint64_t m_nKeysIncoming;                 /**< total incoming keys */
        uint64_t m_nKeysOutgoing;                 /**< total outgoing keys */
        uint64_t m_nBitsIncoming;                 /**< total incoming bits */
        uint64_t m_nBitsOutgoing;                 /**< total outgoing bits */
        uint64_t m_nDisclosedBitsIncoming;        /**< total disclosed bits incoming */
        uint64_t m_nDisclosedBitsOutgoing;        /**< total disclosed bits outgoing */
        uint64_t m_nErrorBitsIncoming;            /**< total error bits incoming */
        uint64_t m_nErrorBitsOutgoing;            /**< total error bits outgoing */
    
    } m_cData;
    

    /**
     * ctor
     */
    module_values() { memset(&m_cData, 0, sizeof(m_cData)); }
    
    
    /**
     * ctor
     * 
     * @param   cProperties         module properties
     */
    module_values(qkd::utility::properties const & cProperties) { 
        
        m_cTimestamp = std::chrono::system_clock::now();
        
        m_cData.m_nKeysIncoming = std::stoll(cProperties.at("keys_incoming"));
        m_cData.m_nKeysOutgoing = std::stoll(cProperties.at("keys_outgoing"));
        m_cData.m_nBitsIncoming = std::stoll(cProperties.at("key_bits_incoming"));
        m_cData.m_nBitsOutgoing = std::stoll(cProperties.at("key_bits_outgoing"));
        m_cData.m_nDisclosedBitsIncoming = std::stoll(cProperties.at("disclosed_bits_incoming"));
        m_cData.m_nDisclosedBitsOutgoing = std::stoll(cProperties.at("disclosed_bits_outgoing"));
        m_cData.m_nErrorBitsIncoming = std::stoll(cProperties.at("error_bits_incoming"));
        m_cData.m_nErrorBitsOutgoing = std::stoll(cProperties.at("error_bits_outgoing"));
    }
    
    
    /**
     * check if these are the initial values
     * 
     * @return  true, if these are the initial values
     */
    bool is_initial() const {
        return m_cTimestamp.time_since_epoch().count() == 0;
    }
};



/**
 * the data to plot
 */
struct plot_data {
    
    double nTimeStamp[PLOT_RANGE];      /**< timestamps */
    double nIncoming[PLOT_RANGE * 2];   /**< incoming per timestamp */
    double nOutgoing[PLOT_RANGE * 2];   /**< outgoing per timestamp */
    unsigned int nIndex;                /**< last value */
    
    
    /**
     * time stamp 
     */
    std::chrono::system_clock::time_point cTimestamp;
};


/**
 * the module pimpl
 */
class qkd::widget::module_frame::module_frame_data {
    
    
public:

    
    /**
     * ctor
     */
    module_frame_data() : m_cDBus(QDBusConnection::sessionBus()) {
        
        // prefetch pixmaps
        m_cPixAlice = qkd::widget::res::pixmap("alice");
        m_cPixBob   = qkd::widget::res::pixmap("bob");
        m_cPixPause = qkd::widget::res::pixmap("media_playback_pause");
        m_cPixRun   = qkd::widget::res::pixmap("media_playback_start");
        m_cPixStop  = qkd::widget::res::pixmap("media_playback_stop");
        
        m_cRates.m_nKeysIncomingRate = 0.0;
        m_cRates.m_nKeysOutgoingRate = 0.0;
        m_cRates.m_nBitsIncomingRate = 0.0;
        m_cRates.m_nBitsOutgoingRate = 0.0;
        m_cRates.m_nDisclosedBitsIncomingRate = 0.0;
        m_cRates.m_nDisclosedBitsOutgoingRate = 0.0;
        m_cRates.m_nErrorBitsIncomingRate = 0.0;
        m_cRates.m_nErrorBitsOutgoingRate = 0.0;
    };
    
    
    QDBusConnection m_cDBus;                                /**< DBus connection used */
    qkd::utility::properties m_cProperties;                 /**< recently set properties */
    
    QPixmap m_cPixAlice;                                    /**< alice pixmap */
    QPixmap m_cPixBob;                                      /**< bob pixmap */
    
    QPixmap m_cPixPause;                                    /**< pause pixmap */
    QPixmap m_cPixRun;                                      /**< run pixmap */
    QPixmap m_cPixStop;                                     /**< stop pixmap */
    
    module_values m_cModuleValues;                          /**< statistical current I/O data */

    struct {
        
        double m_nKeysIncomingRate;                         /**< rate of incoming keys */
        double m_nKeysOutgoingRate;                         /**< rate of outgoing keys */
        double m_nBitsIncomingRate;                         /**< rate of incoming bits */
        double m_nBitsOutgoingRate;                         /**< rate of outgoing bits */
        double m_nDisclosedBitsIncomingRate;                /**< rate of disclosed bits incoming */
        double m_nDisclosedBitsOutgoingRate;                /**< rate of disclosed bits outgoing */
        double m_nErrorBitsIncomingRate;                    /**< rate of error bits incoming */
        double m_nErrorBitsOutgoingRate;                    /**< rate of error bits outgoing */
        
        double m_nQBER;                                     /**< current QBER */
    
    } m_cRates;
    
    plot_data m_cPlotKeys;                                  /**< plot data for keys I/O */
    plot_data m_cPlotBits;                                  /**< plot data for bits I/O */
    plot_data m_cPlotQBER;                                  /**< plot data for QBER I/O */
    
    QwtPlotCurve * m_cPlCrvKeysIn;                          /**< plot curve for keys in */
    QwtPlotCurve * m_cPlCrvKeysOut;                         /**< plot curve for keys out */
    QwtPlotCurve * m_cPlCrvBitsIn;                          /**< plot curve for bits in */
    QwtPlotCurve * m_cPlCrvBitsOut;                         /**< plot curve for bits out */
    QwtPlotCurve * m_cPlCrvQBER;                            /**< plot curve for qber out */
};


// cleat plot data
void plot_data_clear(plot_data & cPlotData);


// create a "tick" on a plot
void plot_data_tick(plot_data & cPlotData, double nIncoming, double nOutgoing);


// set if modified
void test_and_set(QLineEdit * cEd, std::string sText);


// ------------------------------------------------------------
// code


/**
 * ctor
 * 
 * @param   cParent     parent object
 * @param   cDBus       DBus session object where to module resides
 */
module_frame::module_frame(QWidget * cParent, QDBusConnection cDBus) : QFrame(cParent) {
    
    d = std::shared_ptr<qkd::widget::module_frame::module_frame_data>(new qkd::widget::module_frame::module_frame_data());
    plot_data_clear(d->m_cPlotKeys);
    plot_data_clear(d->m_cPlotBits);
    plot_data_clear(d->m_cPlotQBER);
    
    d->m_cDBus = cDBus;
    
    m_cUI = new Ui::module_frame;
    m_cUI->setupUi(this);
    m_cUI->cLbStatus->setMinimumWidth(100);
    
    m_cUI->cLedStatus = qkd::widget::res::swap_widget(m_cUI->cLyHeader, m_cUI->cLedStatus, new qkd::widget::led());

    m_cUI->cLcdKeysIn = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabKeysValues, m_cUI->cLcdKeysIn, new qkd::widget::lcd("0")));
    m_cUI->cLcdKeysInRate = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabKeysValues, m_cUI->cLcdKeysInRate, new qkd::widget::lcd("0")));
    m_cUI->cLcdKeysInRate->setAlignment((Qt::Alignment)(Qt::AlignVCenter + Qt::AlignRight));
    m_cUI->cLcdKeysOut = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabKeysValues, m_cUI->cLcdKeysOut, new qkd::widget::lcd("0")));
    m_cUI->cLcdKeysOutRate = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabKeysValues, m_cUI->cLcdKeysOutRate, new qkd::widget::lcd("0")));
    m_cUI->cLcdKeysOutRate->setAlignment((Qt::Alignment)(Qt::AlignVCenter + Qt::AlignRight));
    m_cUI->cLcdBitsIn = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabBitsValues, m_cUI->cLcdBitsIn, new qkd::widget::lcd("0")));
    m_cUI->cLcdBitsInRate = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabBitsValues, m_cUI->cLcdBitsInRate, new qkd::widget::lcd("0")));
    m_cUI->cLcdBitsInRate->setAlignment((Qt::Alignment)(Qt::AlignVCenter + Qt::AlignRight));
    m_cUI->cLcdBitsOut = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabBitsValues, m_cUI->cLcdBitsOut, new qkd::widget::lcd("0")));
    m_cUI->cLcdBitsOutRate = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabBitsValues, m_cUI->cLcdBitsOutRate, new qkd::widget::lcd("0")));
    m_cUI->cLcdBitsOutRate->setAlignment((Qt::Alignment)(Qt::AlignVCenter + Qt::AlignRight));
    m_cUI->cLcdQBER = dynamic_cast<QLineEdit *>(qkd::widget::res::swap_widget(m_cUI->cLyTabQBERValue, m_cUI->cLcdQBER, new qkd::widget::lcd("0")));
    m_cUI->cLcdQBER->setAlignment((Qt::Alignment)(Qt::AlignVCenter + Qt::AlignRight));
    
    m_cUI->cPlKeys = dynamic_cast<QwtPlot *>(qkd::widget::res::swap_widget(m_cUI->cLyTabKeys, m_cUI->cPlKeys, new qkd::widget::plot()));
    m_cUI->cPlBits = dynamic_cast<QwtPlot *>(qkd::widget::res::swap_widget(m_cUI->cLyTabBits, m_cUI->cPlBits, new qkd::widget::plot()));
    m_cUI->cPlQBER = dynamic_cast<QwtPlot *>(qkd::widget::res::swap_widget(m_cUI->cLyTabQBER, m_cUI->cPlQBER, new qkd::widget::plot()));
    
    QPixmap cPix;
    cPix = qkd::widget::res::pixmap("module_pipe_in").scaled(24, 24);
    m_cUI->cLbUrlPipeInIcon->setPixmap(cPix);
    cPix = qkd::widget::res::pixmap("module_peer").scaled(24, 24);
    m_cUI->cLbUrlPeerIcon->setPixmap(cPix);
    cPix = qkd::widget::res::pixmap("module_pipe_out").scaled(24, 24);
    m_cUI->cLbUrlPipeOutIcon->setPixmap(cPix);
    
    m_cUI->cBtnResume->setIcon(d->m_cPixPause);
    m_cUI->cBtnStop->setIcon(d->m_cPixStop);
    
    QBrush cBackgroundBrush(palette().color(QPalette::Base));
    m_cUI->cPlKeys->setCanvasBackground(cBackgroundBrush);
    m_cUI->cPlBits->setCanvasBackground(cBackgroundBrush);
    m_cUI->cPlQBER->setCanvasBackground(cBackgroundBrush);
    
    m_cUI->cPlKeys->axisWidget(QwtPlot::Axis::yLeft)->scaleDraw()->setMinimumExtent(m_cUI->cLbKeysIn->minimumWidth());
    m_cUI->cPlBits->axisWidget(QwtPlot::Axis::yLeft)->scaleDraw()->setMinimumExtent(m_cUI->cLbBitsIn->minimumWidth());
    m_cUI->cPlQBER->axisWidget(QwtPlot::Axis::yLeft)->scaleDraw()->setMinimumExtent(m_cUI->cLbQBER->minimumWidth());

    QPen cPenMinorGridCharge = QPen(Qt::gray);
    cPenMinorGridCharge.setStyle(Qt::DotLine);
    
    QwtPlotGrid * cPlGridKeys = new QwtPlotGrid;
    cPlGridKeys->enableXMin(true);
    cPlGridKeys->enableYMin(true);
#if QWT_VERSION < 0x060100     
    cPlGridKeys->setMinPen(cPenMinorGridCharge);
#else
    cPlGridKeys->setMinorPen(cPenMinorGridCharge);
#endif    
    cPlGridKeys->attach(m_cUI->cPlKeys);
    
    QwtPlotGrid * cPlGridBits = new QwtPlotGrid;
    cPlGridBits->enableXMin(true);
    cPlGridBits->enableYMin(true);
#if QWT_VERSION < 0x060100     
    cPlGridBits->setMinPen(cPenMinorGridCharge);
#else
    cPlGridBits->setMinorPen(cPenMinorGridCharge);
#endif
    cPlGridBits->attach(m_cUI->cPlBits);

    QwtPlotGrid * cPlGridQBER = new QwtPlotGrid;
    cPlGridQBER->enableXMin(true);
    cPlGridQBER->enableYMin(true);
#if QWT_VERSION < 0x060100     
    cPlGridQBER->setMinPen(cPenMinorGridCharge);
#else
    cPlGridQBER->setMinorPen(cPenMinorGridCharge);
#endif
    cPlGridQBER->attach(m_cUI->cPlQBER);
    
    QPen cPenIncoming = QPen(Qt::blue);
    cPenIncoming.setCapStyle(Qt::RoundCap);
    cPenIncoming.setJoinStyle(Qt::RoundJoin);
    cPenIncoming.setWidth(2);
    QPen cPenOutgoing = QPen(Qt::green);
    cPenOutgoing.setCapStyle(Qt::RoundCap);
    cPenOutgoing.setJoinStyle(Qt::RoundJoin);
    cPenOutgoing.setWidth(2);
    QPen cPenQBER = QPen(Qt::red);
    cPenQBER.setCapStyle(Qt::RoundCap);
    cPenQBER.setJoinStyle(Qt::RoundJoin);
    cPenQBER.setWidth(2);

    d->m_cPlCrvKeysIn = new QwtPlotCurve(tr("keys per second incoming"));
    d->m_cPlCrvKeysIn->setPen(cPenIncoming);
    d->m_cPlCrvKeysIn->attach(m_cUI->cPlKeys);
    d->m_cPlCrvKeysOut = new QwtPlotCurve(tr("keys per second outgoing"));
    d->m_cPlCrvKeysOut->setPen(cPenOutgoing);
    d->m_cPlCrvKeysOut->attach(m_cUI->cPlKeys);
    
    d->m_cPlCrvBitsIn = new QwtPlotCurve(tr("bps incoming"));
    d->m_cPlCrvBitsIn->setPen(cPenIncoming);
    d->m_cPlCrvBitsIn->attach(m_cUI->cPlBits);
    d->m_cPlCrvBitsOut = new QwtPlotCurve(tr("bps outgoing"));
    d->m_cPlCrvBitsOut->setPen(cPenOutgoing);
    d->m_cPlCrvBitsOut->attach(m_cUI->cPlBits);
    
    d->m_cPlCrvQBER = new QwtPlotCurve(tr("QBER"));
    d->m_cPlCrvQBER->setPen(cPenQBER);
    d->m_cPlCrvQBER->attach(m_cUI->cPlQBER);
    
    connect(m_cUI->cCkDebug, SIGNAL(stateChanged(int)), SLOT(apply_debug(int)));
    connect(m_cUI->cBtnHint, SIGNAL(clicked()), SLOT(apply_hint()));
    connect(m_cUI->cBtnPipeline, SIGNAL(clicked()), SLOT(apply_pipeline()));
    connect(m_cUI->cBtnRefresh, SIGNAL(clicked()), SLOT(refresh_ui()));
    connect(m_cUI->cBtnResume, SIGNAL(clicked()), SLOT(clicked_resume()));
    connect(m_cUI->cBtnStop, SIGNAL(clicked()), SLOT(clicked_stop()));
    connect(m_cUI->cBtnUrlPipeIn, SIGNAL(clicked()), SLOT(apply_url_in()));
    connect(m_cUI->cBtnUrlPipeOut, SIGNAL(clicked()), SLOT(apply_url_out()));
    connect(m_cUI->cBtnUrlPeer, SIGNAL(clicked()), SLOT(apply_url_peer()));
}


/**
 * dtor
 */
module_frame::~module_frame() {
    delete m_cUI;
}


/**
 * apply new debug state
 * 
 * @param   nState      new Qt::CheckState var
 */
void module_frame::apply_debug(int nState) {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    if (nState == Qt::Checked) {
        cMessage << "at.ac.ait.qkd.module" << "debug" << QVariant::fromValue(QDBusVariant(true)); 
    }
    else {
        cMessage << "at.ac.ait.qkd.module" << "debug" << QVariant::fromValue(QDBusVariant(false)); 
    }
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * apply new hint
 */
void module_frame::apply_hint() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    cMessage << "at.ac.ait.qkd.module" << "hint" << QVariant::fromValue(QDBusVariant(m_cUI->cEdHint->text())); 
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * apply new pipeline
 */
void module_frame::apply_pipeline() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    cMessage << "at.ac.ait.qkd.module" << "pipeline" << QVariant::fromValue(QDBusVariant(m_cUI->cEdPipeline->text())); 
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * apply new url in
 */
void module_frame::apply_url_in() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    cMessage << "at.ac.ait.qkd.module" << "url_pipe_in" << QVariant::fromValue(QDBusVariant(m_cUI->cEdUrlPipeIn->text())); 
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * apply new url out
 */
void module_frame::apply_url_out() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    cMessage << "at.ac.ait.qkd.module" << "url_pipe_out" << QVariant::fromValue(QDBusVariant(m_cUI->cEdUrlPipeOut->text())); 
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * apply new url peer
 */
void module_frame::apply_url_peer() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "org.freedesktop.DBus.Properties", "Set");
    if (d->m_cProperties.at("role") == "0") {
        cMessage << "at.ac.ait.qkd.module" << "url_peer" << QVariant::fromValue(QDBusVariant(m_cUI->cEdUrlPipeIn->text())); 
    }
    else {
        cMessage << "at.ac.ait.qkd.module" << "url_listen" << QVariant::fromValue(QDBusVariant(m_cUI->cEdUrlPeer->text())); 
    }
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * clicked resume button
 */
void module_frame::clicked_resume() {
    if (d->m_cProperties.at("state_name") == "running") pause();
    else resume();
}


/**
 * clicked stop button
 */
void module_frame::clicked_stop() {
    terminate();
}


/**
 * return the DBus Address of this module frame
 */
std::string module_frame::dbus() const {
    return m_cUI->cEdDBus->text().toStdString();
}


/**
 * refresh the last values
 * 
 * this places the reset properties into the UI
 */
void module_frame::refresh_ui() {
    
    bool bAlice = d->m_cProperties.at("role") == "0";
    bool bDebug = d->m_cProperties.at("debug") == "true";
    
    test_and_set(m_cUI->cEdId,              d->m_cProperties.at("id"));
    test_and_set(m_cUI->cEdDBus,            d->m_cProperties.at("dbus"));
    test_and_set(m_cUI->cEdDescription,     d->m_cProperties.at("description"));
    test_and_set(m_cUI->cEdOrganisation,    d->m_cProperties.at("organisation"));
    test_and_set(m_cUI->cEdPipeline,        d->m_cProperties.at("pipeline"));
    test_and_set(m_cUI->cEdHint,            d->m_cProperties.at("hint"));
    test_and_set(m_cUI->cEdProcessImage,    d->m_cProperties.at("process_image"));
    
    uint64_t nStartDateTime = std::stoll(d->m_cProperties.at("start_time"));
    QDateTime cStartDateTime = QDateTime::fromTime_t(nStartDateTime);
    std::string sStartDateTime = QString("Unix epoch: %1 [%2]").arg(nStartDateTime).arg(cStartDateTime.toString(Qt::DefaultLocaleLongDate)).toStdString();
    test_and_set(m_cUI->cEdProcessStart,    sStartDateTime);
    
    if (bAlice) {
        m_cUI->cLbRole->setPixmap(d->m_cPixAlice);
        m_cUI->cLbRoleName->setText("Alice");
    }
    else {
        m_cUI->cLbRole->setPixmap(d->m_cPixBob);
        m_cUI->cLbRoleName->setText("Bob");
    }
    
    if (bDebug) m_cUI->cCkDebug->setCheckState(Qt::Checked);
    else m_cUI->cCkDebug->setCheckState(Qt::Unchecked);
    
    test_and_set(m_cUI->cEdUrlPipeIn,       d->m_cProperties.at("url_pipe_in"));
    test_and_set(m_cUI->cEdUrlPipeOut,      d->m_cProperties.at("url_pipe_out"));
    if (bAlice) {
        test_and_set(m_cUI->cEdUrlPeer,     d->m_cProperties.at("url_peer"));
    }
    else {
        test_and_set(m_cUI->cEdUrlPeer,     d->m_cProperties.at("url_listen"));
    }
}


/**
 * pause the module
 */
void module_frame::pause() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "at.ac.ait.qkd.module", "pause");
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * run/resume the module
 */
void module_frame::resume() {

    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "at.ac.ait.qkd.module", "resume");
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * get the included tab widget
 * 
 * @return  the tab widget used
 */
QTabWidget * module_frame::tab() {
    return m_cUI->cTbPlots;
}


/**
 * terminate the module
 */
void module_frame::terminate() {
    
    QString sDBusObject = QString::fromStdString(d->m_cProperties.at("dbus"));
    QDBusMessage cMessage = QDBusMessage::createMethodCall(sDBusObject, "/Module", "at.ac.ait.qkd.module", "terminate");
    d->m_cDBus.call(cMessage, QDBus::NoBlock);
}


/**
 * update the data shown
 * 
 * the given properties as retrieved as by 
 * qkd::utility::investigation for the modules
 * 
 * @param   cProperties     new properties of the module
 */
void module_frame::update(qkd::utility::properties const & cProperties) {
    
    d->m_cProperties = cProperties;
    
    module_values cNewValues;
    
    try {
        cNewValues =  module_values(cProperties);
    }
    catch (std::exception const & e) {
        qkd::utility::debug() << "Failed to read module's statistical data from DBus investigation: " << e.what();
        return;
    }
    
    if (d->m_cModuleValues.is_initial()) {
        d->m_cModuleValues = cNewValues;
        memset(&d->m_cRates, 0, sizeof(d->m_cRates));
    }
    else {
        
        double nKeysIncomingDiff = cNewValues.m_cData.m_nKeysIncoming - d->m_cModuleValues.m_cData.m_nKeysIncoming; 
        double nKeysOutgoingDiff = cNewValues.m_cData.m_nKeysOutgoing - d->m_cModuleValues.m_cData.m_nKeysOutgoing; 
        double nBitsIncomingDiff = cNewValues.m_cData.m_nBitsIncoming - d->m_cModuleValues.m_cData.m_nBitsIncoming; 
        double nBitsOutgoingDiff = cNewValues.m_cData.m_nBitsOutgoing - d->m_cModuleValues.m_cData.m_nBitsOutgoing; 
        double nDisclosedBitsIncomingDiff = cNewValues.m_cData.m_nDisclosedBitsIncoming - d->m_cModuleValues.m_cData.m_nDisclosedBitsIncoming; 
        double nDisclosedBitsOutgoingDiff = cNewValues.m_cData.m_nDisclosedBitsOutgoing - d->m_cModuleValues.m_cData.m_nDisclosedBitsOutgoing; 
        double nErrorBitsIncomingDiff = cNewValues.m_cData.m_nErrorBitsIncoming - d->m_cModuleValues.m_cData.m_nErrorBitsIncoming; 
        double nErrorBitsOutgoingDiff = cNewValues.m_cData.m_nErrorBitsOutgoing - d->m_cModuleValues.m_cData.m_nErrorBitsOutgoing; 
        
        double nSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(cNewValues.m_cTimestamp - d->m_cModuleValues.m_cTimestamp).count() / 1000.0;
        
        d->m_cRates.m_nKeysIncomingRate = nKeysIncomingDiff / nSeconds;
        d->m_cRates.m_nKeysOutgoingRate = nKeysOutgoingDiff / nSeconds;
        d->m_cRates.m_nBitsIncomingRate = nBitsIncomingDiff / nSeconds;
        d->m_cRates.m_nBitsOutgoingRate = nBitsOutgoingDiff / nSeconds;
        d->m_cRates.m_nDisclosedBitsIncomingRate = nDisclosedBitsIncomingDiff / nSeconds;
        d->m_cRates.m_nDisclosedBitsOutgoingRate = nDisclosedBitsOutgoingDiff / nSeconds;
        d->m_cRates.m_nErrorBitsIncomingRate = nErrorBitsIncomingDiff / nSeconds;
        d->m_cRates.m_nErrorBitsOutgoingRate = nErrorBitsOutgoingDiff / nSeconds;
        
        if (nBitsOutgoingDiff != 0.0) {
            d->m_cRates.m_nQBER = (double)nErrorBitsOutgoingDiff / (double)nBitsOutgoingDiff;
        }
        else {
            d->m_cRates.m_nQBER = 0.0;
        }
        
        d->m_cModuleValues = cNewValues;
    }
    
    std::string sState = d->m_cProperties.at("state_name");
    m_cUI->cLbStatus->setText(QString::fromStdString(sState));
    if (sState == "running") m_cUI->cBtnResume->setIcon(d->m_cPixPause);
    else m_cUI->cBtnResume->setIcon(d->m_cPixRun);
    
    qkd::widget::led * cLedStatus = dynamic_cast<qkd::widget::led *>(m_cUI->cLedStatus);
    switch (std::stoi(d->m_cProperties.at("state"))) {
        
    case qkd::module::module_state::STATE_READY:
        cLedStatus->set_state(qkd::widget::led::led_state::LED_STATE_GREEN);
        cLedStatus->set_blinking(true);
        break;
        
    case qkd::module::module_state::STATE_RUNNING:
        cLedStatus->set_state(qkd::widget::led::led_state::LED_STATE_GREEN);
        cLedStatus->set_blinking(false);
        break;
        
    case qkd::module::module_state::STATE_TERMINATING:
        cLedStatus->set_state(qkd::widget::led::led_state::LED_STATE_RED);
        cLedStatus->set_blinking(true);
        break;
        
    case qkd::module::module_state::STATE_TERMINATED:
        cLedStatus->set_state(qkd::widget::led::led_state::LED_STATE_RED);
        cLedStatus->set_blinking(false);
        break;

    case qkd::module::module_state::STATE_NEW:
    default:
        cLedStatus->set_state(qkd::widget::led::led_state::LED_STATE_YELLOW);
        cLedStatus->set_blinking(false);
        break;
        
    }

    uint64_t nSeconds = std::time(nullptr) - std::stoll(d->m_cProperties.at("start_time"));
    m_cUI->cLbUptime->setText(QString("uptime: %1 sec").arg(nSeconds));
    
    update_tab_keys();
    update_tab_bits();
    update_tab_qber();
    
    refresh_ui();
}


/**
 * update bits tab plot
 */
void module_frame::update_tab_bits() {

    m_cUI->cLcdBitsIn->setText(QString::number(d->m_cModuleValues.m_cData.m_nBitsIncoming));
    m_cUI->cLcdBitsInRate->setText(QString::number(d->m_cRates.m_nBitsIncomingRate, 'f', 2));
    m_cUI->cLcdBitsOut->setText(QString::number(d->m_cModuleValues.m_cData.m_nBitsOutgoing));
    m_cUI->cLcdBitsOutRate->setText(QString::number(d->m_cRates.m_nBitsOutgoingRate, 'f', 2));
    plot_data_tick(d->m_cPlotBits, d->m_cRates.m_nBitsIncomingRate, d->m_cRates.m_nBitsOutgoingRate);
    
    double * nBitsPerSecondIncoming = d->m_cPlotBits.nIncoming + d->m_cPlotBits.nIndex - PLOT_RANGE;
    d->m_cPlCrvBitsIn->setRawSamples(d->m_cPlotBits.nTimeStamp, nBitsPerSecondIncoming, PLOT_RANGE);
    double * nBitsPerSecondOutgoing = d->m_cPlotBits.nOutgoing + d->m_cPlotBits.nIndex - PLOT_RANGE;
    d->m_cPlCrvBitsOut->setRawSamples(d->m_cPlotBits.nTimeStamp, nBitsPerSecondOutgoing, PLOT_RANGE);
    
    QwtScaleEngine * cScaleEngine = m_cUI->cPlBits->axisScaleEngine(QwtPlot::Axis::yLeft);
    if ((cScaleEngine->lowerMargin() == 0.0) && (cScaleEngine->upperMargin() == 0.0)) cScaleEngine->setMargins(0.0, 1.0);
    
    m_cUI->cPlBits->replot();
}


/**
 * update keys tab plot
 */
void module_frame::update_tab_keys() {
    
    m_cUI->cLcdKeysIn->setText(QString::number(d->m_cModuleValues.m_cData.m_nKeysIncoming));
    m_cUI->cLcdKeysInRate->setText(QString::number(d->m_cRates.m_nKeysIncomingRate, 'f', 2));
    m_cUI->cLcdKeysOut->setText(QString::number(d->m_cModuleValues.m_cData.m_nKeysOutgoing));
    m_cUI->cLcdKeysOutRate->setText(QString::number(d->m_cRates.m_nKeysOutgoingRate, 'f', 2));
    plot_data_tick(d->m_cPlotKeys, d->m_cRates.m_nKeysIncomingRate, d->m_cRates.m_nKeysOutgoingRate);

    double * nKeysPerSecondIncoming = d->m_cPlotKeys.nIncoming + d->m_cPlotKeys.nIndex - PLOT_RANGE;
    d->m_cPlCrvKeysIn->setRawSamples(d->m_cPlotKeys.nTimeStamp, nKeysPerSecondIncoming, PLOT_RANGE);
    double * nKeysPerSecondOutgoing = d->m_cPlotKeys.nOutgoing + d->m_cPlotKeys.nIndex - PLOT_RANGE;
    d->m_cPlCrvKeysOut->setRawSamples(d->m_cPlotKeys.nTimeStamp, nKeysPerSecondOutgoing, PLOT_RANGE);
    
    QwtScaleEngine * cScaleEngine = m_cUI->cPlKeys->axisScaleEngine(QwtPlot::Axis::yLeft);
    if ((cScaleEngine->lowerMargin() == 0.0) && (cScaleEngine->upperMargin() == 0.0)) cScaleEngine->setMargins(0.0, 1.0);
    
    m_cUI->cPlKeys->replot();
}


/**
 * update qber tab plot
 */
void module_frame::update_tab_qber() {
    
    m_cUI->cLcdQBER->setText(QString::number(d->m_cRates.m_nQBER, 'f', 4));
    plot_data_tick(d->m_cPlotQBER, d->m_cRates.m_nQBER, 0.0);
    
    double * nQBER = d->m_cPlotQBER.nIncoming + d->m_cPlotQBER.nIndex - PLOT_RANGE;
    d->m_cPlCrvQBER->setRawSamples(d->m_cPlotQBER.nTimeStamp, nQBER, PLOT_RANGE);
    
    QwtScaleEngine * cScaleEngine = m_cUI->cPlQBER->axisScaleEngine(QwtPlot::Axis::yLeft);
    if ((cScaleEngine->lowerMargin() == 0.0) && (cScaleEngine->upperMargin() == 0.0)) cScaleEngine->setMargins(0.0, 0.01);
    
    m_cUI->cPlQBER->replot();
}


/**
 * cleat plot data
 * 
 * @param   cPlotData       the plot data to clear
 */
void plot_data_clear(plot_data & cPlotData) {
    
    memset(cPlotData.nTimeStamp, 0, sizeof(double) * PLOT_RANGE);
    memset(cPlotData.nIncoming, 0, sizeof(double) * PLOT_RANGE * 2);
    memset(cPlotData.nOutgoing, 0, sizeof(double) * PLOT_RANGE * 2);
    for (int i = PLOT_RANGE - 1; i >= 0; i--) {
        cPlotData.nTimeStamp[i] = (double)(i - PLOT_RANGE) / (1000.0 / TIMEOUT_MSECS);
    }
    cPlotData.nIndex = PLOT_RANGE;
}


/**
 * create a "tick" on a plot
 * 
 * @param   cPlotData       the plot data
 * @param   nIncoming       new incoming value
 * @param   nOutgoing       new outgoing value
 */
void plot_data_tick(plot_data & cPlotData, double nIncoming, double nOutgoing) {
    
    if (cPlotData.cTimestamp.time_since_epoch().count() == 0) {
    
        // new plot: first sketch
        cPlotData.nIndex = PLOT_RANGE;
        cPlotData.nIncoming[cPlotData.nIndex] = nIncoming;
        cPlotData.nOutgoing[cPlotData.nIndex] = nOutgoing;
        cPlotData.cTimestamp = std::chrono::system_clock::now();
        return;
        
    }
        
    auto cCurrent = std::chrono::system_clock::now();
    auto nTimespan =  std::chrono::duration_cast<std::chrono::milliseconds>(cCurrent - cPlotData.cTimestamp).count();
        
    unsigned long nStep = nTimespan / TIMEOUT_MSECS;
    cPlotData.nIndex += nStep;

    while (cPlotData.nIndex >= 2 * PLOT_RANGE) {
        memcpy(cPlotData.nIncoming, cPlotData.nIncoming + PLOT_RANGE, PLOT_RANGE * sizeof(double));
        memcpy(cPlotData.nOutgoing, cPlotData.nOutgoing + PLOT_RANGE, PLOT_RANGE * sizeof(double));
        cPlotData.nIndex -= PLOT_RANGE;
    }
    
    if (!nStep) {
        
        cPlotData.nIncoming[cPlotData.nIndex] = nIncoming;
        cPlotData.nOutgoing[cPlotData.nIndex] = nOutgoing;
    }
    else {
        
        for (unsigned long i = 0; i < nStep; i++) {
            unsigned nNext = (cPlotData.nIndex - i);
            cPlotData.nIncoming[nNext] = nIncoming;
            cPlotData.nOutgoing[nNext] = nOutgoing;
        }
    }
    
    cPlotData.cTimestamp = cCurrent;
}


/**
 * set the lineedit text value if different
 * 
 * @param   cEd         the lineedit to change
 * @param   sValue      the "new" value
 */
void test_and_set(QLineEdit * cEd, std::string sText) {
    
    if (cEd == nullptr) return;
    
    QString s = QString::fromStdString(sText);
    if (cEd->text() != s) {
        cEd->setText(s);
        cEd->setCursorPosition(0);
    }
}
