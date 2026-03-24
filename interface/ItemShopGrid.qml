import QtQuick
import QtQuick.Controls
import ".."

Item {
	id: item_shop_grid
	width: icon_button_width * item_grid.columns + item_grid.spacing * (item_grid.columns - 1) + 8 * scale_factor * 2
	height: item_grid_view.y + item_grid_view.height + 8 * scale_factor
	
	property var item_slots: []
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
	}
	
	Flickable {
		id: item_grid_view
		anchors.top: item_shop_label.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: icon_button_height * 4 + item_grid.spacing * 3
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
					
					readonly property var item_slot: model.modelData
					readonly property var item: item_slot.item
					
					onClicked: {
						if (item_slot.can_buy_item(metternich.game.player_character)) {
							item_slot.buy_item(metternich.game.player_character)
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
