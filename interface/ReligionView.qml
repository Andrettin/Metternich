import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import "./dialogs"

Item {
	id: religion_view
	
	SmallText {
		id: religion_label
		text: "Religion"
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
	}
	
	CustomIconImage {
		id: religion_icon
		anchors.top: religion_label.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: domain_game_data.religion.icon.identifier
		name: domain_game_data.religion.name
	}
}
