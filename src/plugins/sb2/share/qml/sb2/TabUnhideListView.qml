import QtQuick 1.1

Rectangle {
    id: rootRect
    width: 500
    height: Math.min(600, closeTabButton.height + unhideView.count * 36)
    smooth: true
    focus: true

    opacity: 0
    SequentialAnimation on opacity {
        loops: 1
        PropertyAnimation { to: 1; duration: 100 }
    }

    signal closeRequested()
    signal tabUnhideRequested(string tabClass)

    Keys.onEscapePressed: rootRect.closeRequested()

    color: "#00000000"

    ActionButton {
        id: closeTabButton

        width: 16
        height: 16

        anchors.top: parent.top
        anchors.right: parent.right

        actionIconURL: "image://ThemeIcons/tab-close"
        transparentStyle: true

        onTriggered: rootRect.closeRequested()
    }

    ListView {
        id: unhideView
        anchors.top: closeTabButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: unhideListModel

        delegate: Item {
            width: unhideView.width
            height: 36

            Rectangle {
                id: tabRect
                anchors.fill: parent
                radius: 5
                smooth: true

                border.color: "black"
                border.width: 1

                Keys.onEscapePressed: rootRect.closeRequested()

                gradient: Gradient {
                    GradientStop {
                        position: 0
                        id: upperStop
                        color: "#000000"
                    }
                    GradientStop {
                        position: 1
                        id: lowerStop
                        color: "#42394b"
                    }
                }

                Image {
                    id: tabIconImage
                    source: tabIcon

                    width: 32
                    height: 32
                    smooth: true

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                }

                Text {
                    id: tabNameLabel
                    text: tabName + " (" + tabDescr + ")"

                    color: "lightgrey"

                    anchors.left: tabIconImage.right
                    anchors.leftMargin: 4
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter

                    elide: Text.ElideMiddle
                }

                MouseArea {
                    id: rectMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onReleased: rootRect.tabUnhideRequested(tabClass)
                }

                states: [
                    State {
                        name: "hovered"
                        when: rectMouseArea.containsMouse
                        PropertyChanges { target: tabRect; border.color: "#ff6500" }
                        PropertyChanges { target: upperStop; color: "#5a3238" }
                        PropertyChanges { target: lowerStop; color: "#290700" }
                    }
                ]

                transitions: [
                    Transition {
                        from: ""
                        to: "hovered"
                        reversible: true
                        PropertyAnimation { properties: "border.color"; duration: 200 }
                        PropertyAnimation { properties: "color"; duration: 200 }
                    }
                ]
            }
        }
    }
}
