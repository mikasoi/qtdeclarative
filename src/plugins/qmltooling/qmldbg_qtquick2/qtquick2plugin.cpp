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

#include "qtquick2plugin.h"
#include "qquickviewinspector.h"

#include <QtCore/qplugin.h>
#include <QtQml/private/qqmlinspectorservice_p.h>
#include <QtQuick/QQuickView>

QT_BEGIN_NAMESPACE

namespace QmlJSDebugger {

QtQuick2Plugin::QtQuick2Plugin() :
    m_inspector(0)
{
}

QtQuick2Plugin::~QtQuick2Plugin()
{
    delete m_inspector;
}

bool QtQuick2Plugin::canHandleView(QObject *view)
{
    return qobject_cast<QQuickView*>(view);
}

void QtQuick2Plugin::activate(QObject *view)
{
    QQuickView *qtQuickView = qobject_cast<QQuickView*>(view);
    Q_ASSERT(qtQuickView);
    m_inspector = new QQuickViewInspector(qtQuickView, qtQuickView);
}

void QtQuick2Plugin::deactivate()
{
    delete m_inspector;
}

void QtQuick2Plugin::clientMessage(const QByteArray &message)
{
    if (m_inspector)
        m_inspector->handleMessage(message);
}

} // namespace QmlJSDebugger

QT_END_NAMESPACE
