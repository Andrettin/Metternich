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
	
	Column {
		id: button_column
		anchors.top: parent.top
		anchors.topMargin: 192 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		IconButton {
			id: end_units_turn_button
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
		
		IconButton {
			id: cast_spell_button
			icon_identifier: "university"
			visible: combat.current_unit && combat.current_unit.character && combat.current_unit.character.game_data.battle_spells.length > 0
			
			onClicked: {
				spell_dialog.caster = combat.current_unit.character
				spell_dialog.open()
				popup_count += 1
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Cast Spell"
				} else {
					status_text = ""
				}
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
