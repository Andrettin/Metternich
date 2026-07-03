import QtQuick
import QtQuick.Controls

Rectangle {
	id: left_bar
	color: interface_background_color
	width: 16 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	MouseArea {
		id: right_bar_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		onContainsMouseChanged: {
			if (containsMouse) {
				status_text = ""
			}
		}
	}
}
