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
		id: decision_button_column
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		spacing: 8 * scale_factor
		visible: politics_view_mode === PoliticsView.Mode.Court
		
		Repeater {
			model: metternich.get_decisions("court")
			
			IconButton {
				id: decision_button
				icon_identifier: decision.icon.identifier
				
				readonly property var decision: model.modelData
				
				onClicked: {
					if (decision.can_be_enacted_by(metternich.game.player_country)) {
						decision.enact_for(metternich.game.player_country)
						update_status_text()
					} else {
						metternich.defines.error_sound.play()
					}
				}
				
				onHoveredChanged: {
					update_status_text()
				}
				
				function update_status_text() {
					if (hovered) {
						status_text = decision.name
						if (!decision.can_be_enacted_by(metternich.game.player_country)) {
							right_status_text = format_text(decision.get_conditions_string(metternich.game.player_country))
						} else {
							right_status_text = format_text(decision.get_effects_string(metternich.game.player_country))
						}
					} else {
						status_text = ""
						right_status_text = ""
					}
				}
			}
		}
	}
	
	Column {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: back_button.top
		anchors.bottomMargin: 16 * scale_factor
		spacing: 8 * scale_factor
		
		IconButton {
			id: inventory_button
			icon_identifier: "sack_3"
			visible: politics_view_mode === PoliticsView.Mode.Court
			
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
