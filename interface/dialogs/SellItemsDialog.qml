import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: sell_items_dialog
	title: "Sell Items"
	width: icon_button_width * inventory_grid.columns + inventory_grid.spacing * (inventory_grid.columns - 1) + 8 * scale_factor * 2
	height: close_button.y + close_button.height + 8 * scale_factor
	
	property var character: null
	readonly property var items: character ? character.game_data.unequipped_items : []
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
							if (character.game_data.can_sell_item(item)) {
								character.game_data.sell_item(item)
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
									middle_status_text = "Click to sell"
								}
								right_status_text = metternich.defines.wealth_commodity.value_to_qstring(item.sell_price)
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
			sell_items_dialog.close()
		}
	}
}
