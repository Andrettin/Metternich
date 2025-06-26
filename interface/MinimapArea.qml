import QtQuick
import QtQuick.Controls

Rectangle {
	id: minimap_area
	color: interface_background_color
	width: 176 * scale_factor
	height: minimap_borders.height
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	Rectangle {
		id: minimap_borders
		anchors.top: minimap.top
		anchors.topMargin: -1 * scale_factor
		anchors.bottom: minimap.bottom
		anchors.bottomMargin: -1 * scale_factor
		anchors.left: minimap.left
		anchors.leftMargin: -1 * scale_factor
		anchors.right: minimap.right
		anchors.rightMargin: -1 * scale_factor
		color: "gray"
	}
	
	Minimap {
		id: minimap
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
	}
}
