import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: inventory_dialog
	title: "Inventory"
	width: icon_button_width * inventory_grid.columns + inventory_grid.spacing * (inventory_grid.columns - 1) + 8 * scale_factor * 2
	height: close_button.y + close_button.height + 8 * scale_factor
	
	property var character: null
	readonly property var items: character ? character.game_data.items : []
	readonly property int icon_button_width: 32 * scale_factor + 6 * scale_factor
	readonly property int icon_button_height: 32 * scale_factor + 6 * scale_factor
	
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true
		
		onEntered: {
			if (typeof status_text !== 'undefined') {
				status_text = ""
				middle_status_text = ""
				right_status_text = ""
			}
		}
	}
	
	ItemSlotButton {
		id: helmet_slot_icon
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		item_slot: metternich.get_item_slot("helmet")
		item_slot_index: 0
		slot_icon_identifier: "crown_baronial"
	}
	
	ItemSlotButton {
		id: armor_slot_icon
		anchors.top: helmet_slot_icon ? helmet_slot_icon.bottom : title_item.bottom
		anchors.topMargin: helmet_slot_icon ? 8 * scale_factor : 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		item_slot: metternich.get_item_slot("armor")
		item_slot_index: 0
		slot_icon_identifier: "clothing"
	}
	
	ItemSlotButton {
		id: belt_slot_icon
		anchors.top: armor_slot_icon ? armor_slot_icon.bottom : (helmet_slot_icon ? helmet_slot_icon.bottom : title_item.bottom)
		anchors.topMargin: armor_slot_icon || helmet_slot_icon ? 8 * scale_factor : 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		item_slot: metternich.get_item_slot("belt")
		item_slot_index: 0
		slot_icon_identifier: "sack_2"
	}
	
	ItemSlotButton {
		id: weapon_slot_icon
		anchors.top: armor_slot_icon.top
		anchors.right: armor_slot_icon.left
		anchors.rightMargin: 8 * scale_factor
		item_slot: metternich.get_item_slot("weapon")
		item_slot_index: 0
		slot_icon_identifier: "saber"
	}
	
	ItemSlotButton {
		id: gloves_slot_icon
		anchors.top: belt_slot_icon.top
		anchors.right: belt_slot_icon.left
		anchors.rightMargin: 8 * scale_factor
		item_slot: metternich.get_item_slot("gloves")
		item_slot_index: 0
		slot_icon_identifier: "sack_2"
	}
	
	ItemSlotButton {
		id: amulet_slot_icon
		anchors.top: helmet_slot_icon.top
		anchors.left: helmet_slot_icon.right
		anchors.leftMargin: 8 * scale_factor
		item_slot: metternich.get_item_slot("amulet")
		item_slot_index: 0
		slot_icon_identifier: "sack_2"
	}
	
	ItemSlotButton {
		id: cloak_slot_icon
		anchors.top: armor_slot_icon.top
		anchors.left: armor_slot_icon.right
		anchors.leftMargin: 8 * scale_factor
		item_slot: metternich.get_item_slot("cloak")
		item_slot_index: 0
		slot_icon_identifier: "clothing"
	}
	
	ItemSlotButton {
		id: shield_slot_icon
		anchors.top: cloak_slot_icon.top
		anchors.left: cloak_slot_icon.right
		anchors.leftMargin: 8 * scale_factor
		item_slot: metternich.get_item_slot("shield")
		item_slot_index: 0
		slot_icon_identifier: "heater_shield"
	}
	
	Flickable {
		id: inventory_grid_view
		anchors.top: belt_slot_icon ? belt_slot_icon.bottom : (armor_slot_icon ? armor_slot_icon.bottom : (helmet_slot_icon ? helmet_slot_icon.bottom : title_item.bottom))
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: Math.min(icon_button_height * 4 + inventory_grid.spacing * 3, contentHeight)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		visible: contentHeight > 0
		
		Grid {
			id: inventory_grid
			columns: 5
			spacing: 8 * scale_factor
			
			Repeater {
				model: items
				
				IconButton {
					id: item_icon
					icon_identifier: item.icon.identifier
					tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(item.name))
					visible: item.equipped === false
					
					readonly property var item: model.modelData
					
					SmallText {
						id: stack_size_label
						anchors.top: parent.top
						anchors.topMargin: 4 * scale_factor
						anchors.left: parent.left
						anchors.leftMargin: 4 * scale_factor
						text: item.quantity
						visible: item.type.stackable && item.quantity > 1
					}
					
					onClicked: {
						if (character === metternich.game.player_character) {
							if (character.game_data.can_use_item(item)) {
								character.game_data.use_item(item)
							} else {
								metternich.defines.error_sound.play()
							}
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = item.name
								if (character === metternich.game.player_character) {
									if (item.type.item_class.slot !== null && character.game_data.can_equip_item(item, true)) {
										middle_status_text = "Click to equip"
									} else if (item.type.item_class.consumable && character.game_data.can_consume_item(item)) {
										if (item.spell !== null && item.type.spell_learnable) {
											middle_status_text = "Click to learn " + item.spell.name
										} else {
											middle_status_text = "Click to " + item.type.item_class.consume_verb
										}
									}
								}
								right_status_text = item.get_effects_string(character)
							} else {
								status_text = ""
								middle_status_text = ""
								right_status_text = ""
							}
						}
					}
				}
			}
		}
	}
	
	Row {
		id: button_row
		anchors.top: inventory_grid_view.visible ? inventory_grid_view.bottom : (belt_slot_icon ? belt_slot_icon.bottom : (armor_slot_icon ? armor_slot_icon.bottom : (helmet_slot_icon ? helmet_slot_icon.bottom : title_item.bottom)))
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		IconButton {
			id: buy_items_button
			icon_identifier: "sack_3"
			visible: inventory_dialog.character === metternich.game.player_character
			
			onClicked: {
				item_shop_dialog.item_slots = country ? country.game_data.item_slots : []
				item_shop_dialog.open()
				item_shop_dialog.receive_focus()
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Buy Items"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: sell_items_button
			icon_identifier: "chest"
			visible: inventory_dialog.character === metternich.game.player_character
			
			onClicked: {
				sell_items_dialog.character = metternich.game.player_character
				sell_items_dialog.open()
				sell_items_dialog.receive_focus()
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Sell Items"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: recipes_button
			icon_identifier: "cog"
			visible: inventory_dialog.character && inventory_dialog.character.game_data.recipes.length > 0
			
			onClicked: {
				recipe_dialog.crafter = inventory_dialog.character
				recipe_dialog.open()
				recipe_dialog.receive_focus()
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Crafting Recipes"
				} else {
					status_text = ""
				}
			}
		}
	}
	
	TextButton {
		id: close_button
		anchors.top: button_row.visible ? button_row.bottom : (inventory_grid_view.visible ? inventory_grid_view.bottom : (belt_slot_icon ? belt_slot_icon.bottom : (armor_slot_icon ? armor_slot_icon.bottom : (helmet_slot_icon ? helmet_slot_icon.bottom : title_item.bottom))))
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Close"
		onClicked: {
			inventory_dialog.close()
		}
	}
}
