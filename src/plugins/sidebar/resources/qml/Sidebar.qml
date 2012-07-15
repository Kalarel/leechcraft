import QtQuick 1.0

Rectangle {
    id: rootRect
    anchors.fill: parent
    smooth: true

    color: "#ee000000"

    /*
    ListView {
        id: tabOpenList
        model: tabOpenListModel

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
    }

    ListView {
        id: currentTabList
        model: currentTabModel

        anchors.left: parent.left
        anchors.top: tabOpenList.bottom
        anchors.right: parent.right
    }
    */

    ListView {
        id: quickLaunchList
        model: quickLaunchModel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        spacing: 2

        delegate: Rectangle {
            color: "#00000000"
            width: quickLaunchList.width
            height: 32

            Image {
                source: actionIcon
                anchors.fill: parent

                width: parent.width
                height: parent.height
                fillMode: Image.PreserveAspectFit
            }
        }
    }
}
