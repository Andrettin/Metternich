import QtQuick
import QtQuick.Controls
import ".."

Item {
	id: item_shop_grid
	width: icon_button_width * item_grid.columns + item_grid.spacing * (item_grid.columns - 1) + 8 * scale_factor * 2
	height: item_grid_view.y + item_grid_view.height + 8 * scale_factor
	
	property var item_slots: {}
	property bool label_visible: true
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
	
	SmallText {
		id: item_shop_label
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Item Shop"
		visible: item_shop_grid.label_visible
	}
	
	Flickable {
		id: item_grid_view
		anchors.top: item_shop_label.visible ? item_shop_label.bottom : parent.top
		anchors.topMargin: item_shop_label.visible ? 8 * scale_factor : 0
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: Math.min(icon_button_height * 4 + item_grid.spacing * 3, contentHeight)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Grid {
			id: item_grid
			columns: 5
			spacing: 8 * scale_factor
			
			Repeater {
				model: item_slots
				
				IconButton {
					id: item_icon
					icon_identifier: item ? item.icon.identifier : ""
					tooltip: typeof status_text !== 'undefined' ? "" : format_text(small_text(item.name))
					visible: item !== null
					
					readonly property var item_key: model.modelData.key
					property var item_slot_list: model.modelData.value
					readonly property var item_slot: item_slot_list[0]
					readonly property var item: item_slot.item
					
					SmallText {
						id: item_quantity_label
						anchors.top: parent.top
						anchors.topMargin: 4 * scale_factor
						anchors.left: parent.left
						anchors.leftMargin: 4 * scale_factor
						text: item_slot_list.length
						visible: item_slot_list.length > 1
					}
					
					onClicked: {
						var item_slot_index = random(item_slot_list.length)
						var chosen_item_slot = item_slot_list[item_slot_index]
						if (chosen_item_slot.can_buy_item(metternich.game.player_character)) {
							chosen_item_slot.buy_item(metternich.game.player_character)
							var new_item_slot_list = []
							for (var listed_item_slot of item_slot_list) {
								if (listed_item_slot !== chosen_item_slot) {
									new_item_slot_list.push(listed_item_slot)
								}
							}
							item_slot_list = new_item_slot_list
						} else {
							metternich.defines.error_sound.play()
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = item.name
								middle_status_text = metternich.defines.wealth_commodity.value_to_qstring(item.price)
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
}
