import QtQuick
import QtQuick.Controls

Item {
	id: portrait_grid_item
	width: portrait_button.width
	height: portrait_button.height
	
	property var character: null
	property string portrait_identifier: character && character.game_data.portrait ? character.game_data.portrait.identifier : ""
	property string tooltip: ""
	readonly property bool hovered: portrait_button.hovered
	
	signal clicked()
	signal entered()
	signal exited()
	
	CharacterPortraitButton {
		id: portrait_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		portrait_identifier: portrait_grid_item.portrait_identifier
		character: portrait_grid_item.character
		tooltip: portrait_grid_item.tooltip
		visible: true
		
		onClicked: {
			portrait_grid_item.clicked()
		}
		
		onHoveredChanged: {
			if (hovered) {
				portrait_grid_item.entered()
			} else {
				portrait_grid_item.exited()
			}
		}
	}
	
	Image {
		id: garrison_icon
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		source: "image://icon/garrison"
		visible: character !== null && character.game_data.military_unit !== null
		
		readonly property string display_text: character !== null && character.game_data.military_unit !== null ? ("Deployed as " + character.game_data.military_unit.type.name + " to " + character.game_data.military_unit.province.game_data.current_cultural_name) : ""
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			
			onContainsMouseChanged: {
				if (status_text) {
					if (containsMouse) {
						status_text = garrison_icon.display_text
					} else {
						if (status_text === garrison_icon.display_text) {
							status_text = character.game_data.full_name
						}
					}
				}
			}
		}
	}
	
	Image {
		id: civilian_icon
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		source: "image://icon/alliance"
		visible: character !== null && character.game_data.civilian_unit !== null
		
		readonly property string display_text: character !== null && character.game_data.civilian_unit !== null ? ("Deployed as " + character.game_data.civilian_unit.type.name /*+ " to " + character.game_data.civilian_unit.province.game_data.current_cultural_name*/) : ""
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			
			onContainsMouseChanged: {
				if (status_text) {
					if (containsMouse) {
						status_text = civilian_icon.display_text
					} else {
						if (status_text === civilian_icon.display_text) {
							status_text = character.game_data.full_name
						}
					}
				}
			}
		}
	}
}
