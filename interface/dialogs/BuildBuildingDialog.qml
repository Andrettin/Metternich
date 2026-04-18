import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: build_building_dialog
	width: 256 * scale_factor
	height: no_button.y + no_button.height + 8 * scale_factor
	title: building ? ("Build " + building.name + "?") : ""
	
	property var building_slot: null
	property var building: null
	
	SmallText {
		id: text
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: building ? format_text("Do you wish to build a " + building.name + " here?\n\n" + building.get_commodity_costs_string_for_site(building_slot.holding)) : ""
		wrapMode: Text.WordWrap
		horizontalAlignment: Text.AlignHCenter
	}
	
	TextButton {
		id: yes_button
		anchors.top: text.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Yes"
		onClicked: {
			building_slot.build_building(building)
			building_slot = null
			building = null
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
			building_slot = null
			building = null
			build_building_dialog.close()
		}
	}
}
