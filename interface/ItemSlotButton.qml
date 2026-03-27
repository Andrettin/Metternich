import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

IconButton {
	id: item_slot_icon
	icon_identifier: item ? item.icon.identifier : (slot_icon_identifier.length > 0 ? (slot_icon_identifier + "/silhouette") : "")
	tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(item_name))
	visible: character && item_slot && character.species.get_item_slot_count(item_slot) > item_slot_index
	
	property var item_slot: null
	property int item_slot_index: 0
	property string slot_icon_identifier: ""
	property var item: character !== null && item_slot !== null ? character.game_data.get_equipped_item(item_slot, item_slot_index) : null
	readonly property string item_name: item ? item.name : (item_slot ? (item_slot.name + " Slot") : "")
	
	SmallText {
		id: stack_size_label
		anchors.top: parent.top
		anchors.topMargin: 4 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 4 * scale_factor
		text: item_slot_icon.item ? item_slot_icon.item.quantity : ""
		visible: item_slot_icon.item !== null && item_slot_icon.item.type.stackable && item_slot_icon.item.quantity > 1
	}
	
	onClicked: {
		if (character === metternich.game.player_character) {
			if (item !== null) {
				character.game_data.deequip_item(item)
				status_text = item_name
				middle_status_text = ""
				right_status_text = ""
			}
		}
	}
	
	onHoveredChanged: {
		if (typeof status_text !== 'undefined') {
			if (hovered) {
				status_text = item_name
				if (item !== null) {
					if (character === metternich.game.player_character) {
						middle_status_text = "Click to de-equip"
					}
					right_status_text = item.get_effects_string()
				}
			} else {
				status_text = ""
				middle_status_text = ""
				right_status_text = ""
			}
		}
	}
	
	Connections {
		target: character ? character.game_data : null
		
		function onEquipped_item_changed(slot, slot_index) {
			if (slot === item_slot_icon.item_slot && slot_index === item_slot_icon.item_slot_index) {
				item_slot_icon.item = character.game_data.get_equipped_item(item_slot_icon.item_slot, item_slot_icon.item_slot_index)
			}
		}
	}
}
