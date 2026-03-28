import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor + 8 * scale_factor * 2
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	IconButton {
		id: buy_items_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: back_button.top
		anchors.bottomMargin: 16 * scale_factor
		icon_identifier: "sack_3"
		
		onClicked: {
			item_shop_dialog.item_slots = country ? country.game_data.item_slots : []
			item_shop_dialog.open()
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Buy Items"
			} else {
				status_text = ""
			}
		}
	}
	
	TextButton {
		id: back_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Back")
		width: 48 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
}
