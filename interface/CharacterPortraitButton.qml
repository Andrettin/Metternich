import QtQuick
import QtQuick.Controls

PortraitButton {
	id: button
	portrait_identifier: portrait ? portrait.identifier : ""
	visible: character !== null
	
	property var character: null
	readonly property var portrait: character && character.game_data.portrait ? character.game_data.portrait : null
	
	onClicked: {
		if (character === null) {
			return
		}
		
		if (character_dialog.opened && character_dialog.character === character) {
			return
		}
		
		character_dialog.office = null
		character_dialog.character = character
		character_dialog.open()
	}
}
