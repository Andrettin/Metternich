import QtQuick
import QtQuick.Controls

Rectangle {
	id: status_bar
	color: interface_background_color
	height: 16 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: parent.top
		height: 1 * scale_factor
	}
	
	SmallText {
		id: left_status_label
		text: status_text
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 1 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
	}
	
	SmallText {
		id: middle_status_label
		text: middle_status_text
		anchors.bottom: left_status_label.bottom
		anchors.left: left_status_label.left
		anchors.leftMargin: 192 * scale_factor
	}
}
