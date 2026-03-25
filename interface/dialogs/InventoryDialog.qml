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
	readonly property int icon_button_width: 32 * scale_factor
	readonly property int icon_button_height: 32 * scale_factor
	
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
		height: icon_button_height * 4 + inventory_grid.spacing * 3
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
				
				Image {
					id: item_icon
					source: item.icon !== null ? ("image://icon/" + item.icon.identifier) : "image://empty/"
					
					readonly property var item: model.modelData
					readonly property string tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(item.name))
					
					MouseArea {
						id: item_mouse_area
						anchors.fill: parent
						hoverEnabled: true
						acceptedButtons: Qt.LeftButton | Qt.RightButton
						
						onClicked: function(mouse) {
							if (character === metternich.game.player_character) {
								if (mouse.button === Qt.RightButton) {
									if (item.type.item_class.consumable && character.game_data.can_use_item(item)) {
										character.game_data.use_item(item)
									}
								}
							}
						}
						
						onEntered: {
							if (typeof status_text !== 'undefined') {
								status_text = item.name
								if (character === metternich.game.player_character) {
									if (item.type.item_class.consumable && character.game_data.can_use_item(item)) {
										middle_status_text = "Right-click to " + item.type.item_class.consume_verb
									}
								}
								right_status_text = item.get_effects_string()
							}
						}
						
						onExited: {
							if (typeof status_text !== 'undefined') {
								status_text = ""
								middle_status_text = ""
								right_status_text = ""
							}
						}
					}
					
					CustomTooltip {
						text: tooltip
						visible: item_mouse_area.containsMouse && tooltip.length > 0
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
