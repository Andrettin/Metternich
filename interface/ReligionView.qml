import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import "./dialogs"

Item {
	id: religion_view
	
	readonly property var deity_slots: country_game_data.available_deity_slots
	
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
		icon_identifier: country_game_data.religion.icon.identifier
		name: country_game_data.religion.name
	}
	
	IdeaSlotGrid {
		id: major_deity_portrait_grid
		anchors.top: religion_icon.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		entries: filter_deity_slots(deity_slots, true)
	}
	
	IdeaSlotGrid {
		id: minor_deity_portrait_grid
		anchors.top: major_deity_portrait_grid.bottom
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: filter_deity_slots(deity_slots, false)
	}
	
	IdeaDialog {
		id: idea_dialog
	}
	
	IdeaChoiceDialog {
		id: idea_choice_dialog
		title: idea_slot ? ("Choose " + idea_slot.name) : ""
		text: idea_slot ? ("Which " + idea_slot.name.toLowerCase() + " shall we worship?") : ""
	}
	
	function filter_deity_slots(slots, major) {
		var result = []
		
		for (var slot of slots) {
			if (slot.major !== major) {
				continue
			}
			
			result.push(slot)
		}
		
		return result
	}
}
