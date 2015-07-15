/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlinspectorservice.h"
#include "qqmlinspectorinterface.h"
#include "qtquick2plugin.h"

#include <private/qqmlglobal_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>

QT_BEGIN_NAMESPACE

QQmlInspectorServiceImpl::QQmlInspectorServiceImpl(QObject *parent):
    QQmlInspectorService(1, parent), m_currentInspectorPlugin(0)
{
}

void QQmlInspectorServiceImpl::addView(QObject *view)
{
    m_views.append(view);
    updateState();
}

void QQmlInspectorServiceImpl::removeView(QObject *view)
{
    m_views.removeAll(view);
    updateState();
}

void QQmlInspectorServiceImpl::stateChanged(State /*state*/)
{
    QMetaObject::invokeMethod(this, "updateState", Qt::QueuedConnection);
}

void QQmlInspectorServiceImpl::updateState()
{
    if (m_views.isEmpty()) {
        if (m_currentInspectorPlugin) {
            m_currentInspectorPlugin->deactivate();
            m_currentInspectorPlugin = 0;
        }
        return;
    }

    if (state() == Enabled) {
        if (m_inspectorPlugins.isEmpty())
            loadInspectorPlugins();

        if (m_inspectorPlugins.isEmpty()) {
            qWarning() << "QQmlInspector: No plugins found.";
            return;
        }

        m_currentInspectorPlugin = 0;
        foreach (QQmlInspectorInterface *inspector, m_inspectorPlugins) {
            if (inspector->canHandleView(m_views.first())) {
                m_currentInspectorPlugin = inspector;
                break;
            }
        }

        if (!m_currentInspectorPlugin) {
            qWarning() << "QQmlInspector: No plugin available for view '" << m_views.first()->metaObject()->className() << "'.";
            return;
        }
        m_currentInspectorPlugin->activate(this, m_views.first());
    } else {
        if (m_currentInspectorPlugin) {
            m_currentInspectorPlugin->deactivate();
            m_currentInspectorPlugin = 0;
        }
    }
}

void QQmlInspectorServiceImpl::messageReceived(const QByteArray &message)
{
    QMetaObject::invokeMethod(this, "processMessage", Qt::QueuedConnection, Q_ARG(QByteArray, message));
}

void QQmlInspectorServiceImpl::processMessage(const QByteArray &message)
{
    if (m_currentInspectorPlugin)
        m_currentInspectorPlugin->clientMessage(message);
}

void QQmlInspectorServiceImpl::loadInspectorPlugins()
{
    m_inspectorPlugins << new QmlJSDebugger::QtQuick2Plugin;
}

QQmlDebugService *QQmlInspectorServiceFactory::create(const QString &key)
{
    return key == QQmlInspectorServiceImpl::s_key ? new QQmlInspectorServiceImpl(this) : 0;
}

QT_END_NAMESPACE
