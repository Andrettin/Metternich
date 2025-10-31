import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 80 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.left: parent.left
		width: 1 * scale_factor
	}
	
	IconButton {
		id: end_units_turn_button
		anchors.top: parent.top
		anchors.topMargin: 192 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "bell"
		
		onClicked: {
			metternich.game.current_combat.set_target(Qt.point(-1, -1))
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "End Unit's Turn"
			} else {
				status_text = ""
			}
		}
	}
	
	PortraitButton {
		id: autoplay_portrait
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: metternich.defines.war_minister_portrait.identifier
		highlighted: metternich.game.current_combat && metternich.game.current_combat.autoplay_enabled
		
		onClicked: {
			if (metternich.game.current_combat.autoplay_enabled) {
				metternich.game.current_combat.autoplay_enabled = false
			} else {
				metternich.game.current_combat.autoplay_enabled = true
				metternich.game.current_combat.set_target(Qt.point(-1, -1))
			}
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Otto-Play"
			} else {
				status_text = ""
			}
		}
	}
}
