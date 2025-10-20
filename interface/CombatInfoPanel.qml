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
	
	PortraitButton {
		id: autoplay_portrait
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: metternich.defines.war_minister_portrait.identifier
		
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
