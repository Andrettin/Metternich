import QtQuick
import QtQuick.Controls

IconButton {
	id: expand_building_button
	icon_identifier: (building_slot && building_slot.expanding) ? "skull" : "cog"
	visible: building_slot && (building_slot.expanding || building_slot.can_expand())
	
	onReleased: {
		if (building_slot.expanding) {
			building_slot.expanding = false
		} else {
			building_slot.expanding = true
		}
		
		update_status_text()
	}
	
	onHoveredChanged: {
		if (hovered) {
			update_status_text()
		} else {
			status_text = ""
		}
	}
	
	function update_status_text() {
		if (building_slot.expanding) {
			status_text = "Cancel " + building.name + " expansion"
		} else {
			status_text = "Expand " + building.name
		}
	}
}
