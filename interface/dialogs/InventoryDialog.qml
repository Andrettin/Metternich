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
	
	Flickable {
		id: inventory_grid_view
		anchors.top: title_item.bottom
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
					
					readonly property var item: model.modelData
					
					onClicked: {
						if (character === metternich.game.player_character) {
							if (item.type.item_class.consumable && character.game_data.can_use_item(item)) {
								character.game_data.use_item(item)
							}
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = item.name
								if (character === metternich.game.player_character) {
									if (item.type.item_class.consumable && character.game_data.can_use_item(item)) {
										if (item.spell !== null && item.type.spell_learnable) {
											middle_status_text = "Click to learn " + item.spell.name
										} else {
											middle_status_text = "Click to " + item.type.item_class.consume_verb
										}
									}
								}
								right_status_text = item.get_effects_string()
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
	
	TextButton {
		id: close_button
		anchors.top: inventory_grid_view.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Close"
		onClicked: {
			inventory_dialog.close()
		}
	}
}
