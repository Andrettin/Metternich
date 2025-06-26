import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import ".."

Item {
	id: menu
	focus: true

	property string title: ""
	property string music_type: "menu"
	readonly property var title_item: title_text
	
	TiledBackground {
		anchors.fill: parent
		interface_style: "dark_wood"
		frame_count: 4
	}
	
	LargeText {
		id: title_text
		text: highlight(menu.title)
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: 36 * scale_factor
	}
}
