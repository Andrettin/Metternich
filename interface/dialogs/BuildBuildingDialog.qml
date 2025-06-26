import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: build_building_dialog
	width: 256 * scale_factor
	height: no_button.y + no_button.height + 8 * scale_factor
	
	property var building_slot: null
	property var building: null
	
	SmallText {
		id: text
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: building ? ("Do you wish to build a " + building.name + " here?") : ""
	}
	
	TextButton {
		id: yes_button
		anchors.top: text.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Yes"
		onClicked: {
			building_slot.build_building(building)
			build_building_dialog.close()
		}
	}
	
	TextButton {
		id: no_button
		anchors.top: yes_button.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "No"
		onClicked: {
			building_slot.cancel_construction()
			build_building_dialog.close()
		}
	}
}
