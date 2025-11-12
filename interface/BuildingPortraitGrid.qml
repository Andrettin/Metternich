import QtQuick
import QtQuick.Controls
import "./dialogs"

Flickable {
	id: portrait_grid_flickable
	height: ((64 + 2) * visible_rows * scale_factor) + portrait_grid.spacing * (visible_rows - 1)
	contentHeight: contentItem.childrenRect.height
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	property int visible_rows: 3
	property var building_slots: []
	
	Grid {
		id: portrait_grid
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 2
		spacing: 12 * scale_factor
		
		Repeater {
			model: building_slots
			
			PortraitGridItem {
				portrait_identifier: wonder ? wonder.portrait.identifier : (building ? building.portrait.identifier : "building_slot")
				
				readonly property var building_slot: model.modelData
				readonly property var building: building_slot.building
				readonly property var wonder: building_slot.wonder
				
				Image {
					id: under_construction_icon
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.verticalCenter: parent.verticalCenter
					source: "image://icon/cog"
					visible: building_slot.under_construction_building !== null || building_slot.under_construction_wonder !== null
				}
				
				onClicked: {
					if (building !== null && building_slot.modifier_string.length > 0) {
						modifier_dialog.title = wonder ? wonder.name : building.name
						modifier_dialog.modifier_string = building_slot.modifier_string
						modifier_dialog.open()
					}
				}
				
				onEntered: {
					if (wonder !== null) {
						status_text = wonder.name
					} else if (building !== null) {
						status_text = building.name
					} else {
						status_text = building_slot.type.name + " Slot"
						middle_status_text = ""
					}
				}
				
				onExited: {
					status_text = ""
					middle_status_text = ""
				}
			}
		}
	}
}
