import QtQuick
import QtQuick.Controls
import "./menus"

MenuBase {
	id: game_over_view
	title: qsTr("Game Over")
	
	property var character: null
	
	PortraitButton {
		id: portrait
		anchors.top: title_item.bottom
		anchors.topMargin: 32 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: character && character.game_data.portrait ? character.game_data.portrait.identifier : ""
		visible: portrait_identifier.length > 0
		enabled: false
	}
	
	SmallText {
		id: text
		anchors.top: portrait.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		width: 256 * scale_factor
		wrapMode: Text.WordWrap
		text: character ? ("You have died. May your achievements live on, and the name of " + highlight(character.full_name) + " be remembered forever more!") : ""
	}
	
	TextButton {
		id: ok_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("OK")
		width: 96 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			metternich.game.stop()
			menu_stack.pop()
		}
	}
}
