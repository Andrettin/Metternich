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
	
	Column {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: back_button.top
		anchors.bottomMargin: 16 * scale_factor
		spacing: 8 * scale_factor
		
		IconButton {
			id: inventory_button
			icon_identifier: "sack_3"
			
			onClicked: {
				inventory_dialog.character = metternich.game.player_character
				inventory_dialog.open()
				inventory_dialog.receive_focus()
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "View Inventory"
				} else {
					status_text = ""
				}
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
