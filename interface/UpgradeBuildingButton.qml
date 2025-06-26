import QtQuick
import QtQuick.Controls

IconButton {
	id: upgrade_building_button
	icon_identifier: (building_slot && building_slot.under_construction_building) ? "skull" : "cog"
	visible: building_slot && (building_slot.under_construction_building || building_slot.get_buildable_building() !== null)
	
	onReleased: {
		if (building_slot.under_construction_building) {
			building_slot.cancel_construction()
		} else {
			building_slot.build_building(building_slot.get_buildable_building())
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
		if (building_slot.under_construction_building) {
			status_text = "Cancel " + building_slot.under_construction_building.name + " construction"
		} else {
			status_text = "Upgrade to " + building_slot.get_buildable_building().name
		}
	}
}
