/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktaphandler_p.h"
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qstylehints.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTapHandler, "qt.quick.handler.tap")

qreal QQuickTapHandler::m_multiTapInterval(0.0);
// single tap distance is the same as the drag threshold
int QQuickTapHandler::m_mouseMultiClickDistanceSquared(-1);
int QQuickTapHandler::m_touchMultiTapDistanceSquared(-1);

/*!
    \qmltype TapHandler
    \instantiates QQuickTapHandler
    \inherits SinglePointHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Handler for taps and clicks.

    TapHandler is a handler for taps on a touchscreen or clicks on a mouse.

    Detection of a valid tap gesture depends on \l gesturePolicy.
    Note that buttons (such as QPushButton) are often implemented not to care
    whether the press and release occur close together: if you press the button
    and then change your mind, you need to drag all the way off the edge of the
    button in order to cancel the click.  Therefore the default
    \l gesturePolicy is \c TapHandler.ReleaseWithinBounds.  If you want to require
    that the press and release are close together in both space and time,
    set it to \c TapHandler.DragThreshold.

    For multi-tap gestures (double-tap, triple-tap etc.), the distance moved
    must not exceed QPlatformTheme::MouseDoubleClickDistance with mouse and
    QPlatformTheme::TouchDoubleTapDistance with touch, and the time between
    taps must not exceed QStyleHints::mouseDoubleClickInterval().

    \sa MouseArea
*/

QQuickTapHandler::QQuickTapHandler(QObject *parent)
    : QQuickSinglePointHandler(parent)
    , m_pressed(false)
    , m_gesturePolicy(ReleaseWithinBounds)
    , m_tapCount(0)
    , m_longPressThreshold(-1)
    , m_lastTapTimestamp(0.0)
{
    if (m_mouseMultiClickDistanceSquared < 0) {
        m_multiTapInterval = qApp->styleHints()->mouseDoubleClickInterval() / 1000.0;
        m_mouseMultiClickDistanceSquared = QGuiApplicationPrivate::platformTheme()->
                    themeHint(QPlatformTheme::MouseDoubleClickDistance).toInt();
        m_mouseMultiClickDistanceSquared *= m_mouseMultiClickDistanceSquared;
        m_touchMultiTapDistanceSquared = QGuiApplicationPrivate::platformTheme()->
                    themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
        m_touchMultiTapDistanceSquared *= m_touchMultiTapDistanceSquared;
    }
}

QQuickTapHandler::~QQuickTapHandler()
{
}

static bool dragOverThreshold(QQuickEventPoint *point)
{
    QPointF delta = point->scenePosition() - point->scenePressPosition();
    return (QQuickWindowPrivate::dragOverThreshold(delta.x(), Qt::XAxis, point) ||
            QQuickWindowPrivate::dragOverThreshold(delta.y(), Qt::YAxis, point));
}

bool QQuickTapHandler::wantsEventPoint(QQuickEventPoint *point)
{
    // If the user has not violated any constraint, it could be a tap.
    // Otherwise we want to give up the grab so that a competing handler
    // (e.g. DragHandler) gets a chance to take over.
    // Don't forget to emit released in case of a cancel.
    bool ret = false;
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
    case QQuickEventPoint::Released:
        ret = parentContains(point);
        break;
    case QQuickEventPoint::Updated:
        switch (m_gesturePolicy) {
        case DragThreshold:
            ret = !dragOverThreshold(point);
            break;
        case WithinBounds:
            ret = parentContains(point);
            break;
        case ReleaseWithinBounds:
            ret = point->pointId() == this->point().id();
            break;
        }
        break;
    case QQuickEventPoint::Stationary:
        // Never react in any way when the point hasn't moved.
        // In autotests, the point's position may not even be correct, because
        // QTest::touchEvent(window, touchDevice).stationary(1)
        // provides no opportunity to give a position, so it ends up being random.
        break;
    }
    // If this is the grabber, returning false from this function will cancel the grab,
    // so onGrabChanged(this, CancelGrabExclusive, point) and setPressed(false) will be called.
    // But when m_gesturePolicy is DragThreshold, we don't get an exclusive grab, but
    // we still don't want to be pressed anymore.
    if (!ret && point->pointId() == this->point().id() && point->state() != QQuickEventPoint::Stationary)
        setPressed(false, true, point);
    return ret;
}

void QQuickTapHandler::handleEventPoint(QQuickEventPoint *point)
{
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
        setPressed(true, false, point);
        break;
    case QQuickEventPoint::Released:
        if ((point->pointerEvent()->buttons() & acceptedButtons()) == Qt::NoButton)
            setPressed(false, false, point);
        break;
    default:
        break;
    }
}

/*!
    \qmlproperty real TapHandler::longPressThreshold

    The time in seconds that an event point must be pressed in order to
    trigger a long press gesture and emit the \l longPressed() signal.
    If the point is released before this time limit, a tap can be detected
    if the \l gesturePolicy constraint is satisfied. The default value is
    QStyleHints::mousePressAndHoldInterval() converted to seconds.
*/
qreal QQuickTapHandler::longPressThreshold() const
{
    return longPressThresholdMilliseconds() / 1000.0;
}

void QQuickTapHandler::setLongPressThreshold(qreal longPressThreshold)
{
    int ms = qRound(longPressThreshold * 1000);
    if (m_longPressThreshold == ms)
        return;

    m_longPressThreshold = ms;
    emit longPressThresholdChanged();
}

int QQuickTapHandler::longPressThresholdMilliseconds() const
{
    return (m_longPressThreshold < 0 ? QGuiApplication::styleHints()->mousePressAndHoldInterval() : m_longPressThreshold);
}

void QQuickTapHandler::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_longPressTimer.timerId()) {
        m_longPressTimer.stop();
        qCDebug(lcTapHandler) << objectName() << "longPressed";
        emit longPressed();
    }
}

/*!
    \qmlproperty enumeration TapHandler::gesturePolicy

    The spatial constraint for a tap or long press gesture to be recognized,
    in addition to the constraint that the release must occur before
    \l longPressThreshold has elapsed. If these constraints are not satisfied,
    the \l tapped signal is not emitted, and \l tapCount is not incremented.
    If the spatial constraint is violated, \l isPressed transitions immediately
    from true to false, regardless of the time held.

    \value TapHandler.DragThreshold
           The event point must not move significantly. If the mouse, finger
           or stylus moves past the system-wide drag threshold
           (QStyleHints::startDragDistance), the tap gesture is canceled, even
           if the button or finger is still pressed. This policy can be useful
           whenever TapHandler needs to cooperate with other pointer handlers
           (for example \l DragHandler), because in this case TapHandler will
           never grab.

    \value TapHandler.WithinBounds
           If the event point leaves the bounds of the \l target item, the tap
           gesture is canceled. The TapHandler will grab on press, but release
           the grab as soon as the boundary constraint is no longer satisfied.

    \value TapHandler.ReleaseWithinBounds
           (the default value) At the time of release (the mouse button is
           released or the finger is lifted), if the event point is outside
           the bounds of the \l target item, a tap gesture is not recognized.
           This is the default value, because it corresponds to typical button
           behavior: you can cancel a click by dragging outside the button,
           and you can also change your mind by dragging back inside the button
           before release. Note that it's necessary for TapHandler to grab on
           press and retain it until release (greedy grab) in order to detect
           this gesture.
*/
void QQuickTapHandler::setGesturePolicy(QQuickTapHandler::GesturePolicy gesturePolicy)
{
    if (m_gesturePolicy == gesturePolicy)
        return;

    m_gesturePolicy = gesturePolicy;
    emit gesturePolicyChanged();
}

/*!
    \qmlproperty bool TapHandler::pressed
    \readonly

    Holds true whenever the mouse or touch point is pressed,
    and any movement since the press is compliant with the current
    \l gesturePolicy. When the event point is released or the policy is
    violated, \e pressed will change to false.
*/
void QQuickTapHandler::setPressed(bool press, bool cancel, QQuickEventPoint *point)
{
    if (m_pressed != press) {
        qCDebug(lcTapHandler) << objectName() << "pressed" << m_pressed << "->" << press << (cancel ? "CANCEL" : "") << point;
        m_pressed = press;
        connectPreRenderSignal(press);
        updateTimeHeld();
        if (press) {
            m_longPressTimer.start(longPressThresholdMilliseconds(), this);
            m_holdTimer.start();
        } else {
            m_longPressTimer.stop();
            m_holdTimer.invalidate();
        }
        if (press) {
            // on press, grab before emitting changed signals
            if (m_gesturePolicy == DragThreshold)
                setPassiveGrab(point, press);
            else
                setExclusiveGrab(point, press);
        }
        if (!cancel && !press && parentContains(point)) {
            if (point->timeHeld() < longPressThreshold()) {
                // Assuming here that pointerEvent()->timestamp() is in ms.
                qreal ts = point->pointerEvent()->timestamp() / 1000.0;
                if (ts - m_lastTapTimestamp < m_multiTapInterval &&
                        QVector2D(point->scenePosition() - m_lastTapPos).lengthSquared() <
                        (point->pointerEvent()->device()->type() == QQuickPointerDevice::Mouse ?
                         m_mouseMultiClickDistanceSquared : m_touchMultiTapDistanceSquared))
                    ++m_tapCount;
                else
                    m_tapCount = 1;
                qCDebug(lcTapHandler) << objectName() << "tapped" << m_tapCount << "times";
                emit tapped();
                emit tapCountChanged();
                m_lastTapTimestamp = ts;
                m_lastTapPos = point->scenePosition();
            } else {
                qCDebug(lcTapHandler) << objectName() << "tap threshold" << longPressThreshold() << "exceeded:" << point->timeHeld();
            }
        }
        emit pressedChanged();
        if (!press && m_gesturePolicy != DragThreshold) {
            // on release, ungrab after emitting changed signals
            setExclusiveGrab(point, press);
        }
        if (cancel)
            emit canceled(point);
    }
}

void QQuickTapHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    QQuickSinglePointHandler::onGrabChanged(grabber, stateChange, point);
    bool isCanceled = stateChange == QQuickEventPoint::CancelGrabExclusive || stateChange == QQuickEventPoint::CancelGrabPassive;
    if (grabber == this && (isCanceled || point->state() == QQuickEventPoint::Released))
        setPressed(false, isCanceled, point);
}

void QQuickTapHandler::connectPreRenderSignal(bool conn)
{
    if (conn)
        connect(parentItem()->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickTapHandler::updateTimeHeld);
    else
        disconnect(parentItem()->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickTapHandler::updateTimeHeld);
}

void QQuickTapHandler::updateTimeHeld()
{
    emit timeHeldChanged();
}

/*!
    \qmlproperty int TapHandler::tapCount
    \readonly

    The number of taps which have occurred within the time and space
    constraints to be considered a single gesture.  For example, to detect
    a double-tap, you can write:

    \qml
    Rectangle {
        width: 100; height: 30
        signal doubleTap
        TapHandler {
            acceptedButtons: Qt.AllButtons
            onTapped: if (tapCount == 2) doubleTap()
        }
    }
    \endqml
*/

/*!
    \qmlproperty real TapHandler::timeHeld
    \readonly

    The amount of time in seconds that a pressed point has been held, without
    moving beyond the drag threshold. It will be updated at least once per
    frame rendered, which enables rendering an animation showing the progress
    towards an action which will be triggered by a long-press. It is also
    possible to trigger one of a series of actions depending on how long the
    press is held.

    A value of less than zero means no point is being held within this
    handler's \l [QML] Item.
*/

QT_END_NAMESPACE
