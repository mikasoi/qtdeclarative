import QtQuick 2.0

QtObject {
    property string basic: qsTr("hello")
    property string basic2: qsTranslate("CustomContext", "goodbye")

    property string disambiguation: qsTr("hi", "informal 'hello'")
    property string disambiguation2: qsTranslate("CustomContext", "see ya", "informal 'goodbye'")

    property string _noop: QT_TR_NOOP("hello")
    property string _noop2: QT_TRANSLATE_NOOP("CustomContext", "goodbye")
    property string noop: qsTr(_noop)
    property string noop2: qsTranslate("CustomContext", _noop2)

    property string singular: qsTr("%n duck(s)", "", 1)
    property string plural: qsTr("%n duck(s)", "", 2)
}
