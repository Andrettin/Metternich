import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: family_tree_view
	
	property var character: null
	property string status_text: ""
	property string middle_status_text: ""
	property string right_status_text: ""
	
	TiledBackground {
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	PortraitButtonTree {
		id: family_tree
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
		entries: character ? character.get_tree_characters() : []
		delegate: TreePortraitButton {
			readonly property var portrait_character: model.modelData
			
			onClicked: {
				if (character_dialog.opened && character_dialog.character === portrait_character) {
					return
				}
				
				character_dialog.character = portrait_character
				character_dialog.show_family_tree_button = false
				character_dialog.open()
			}
		}
	}
	
	RightBar {
		id: right_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	FamilyTreeInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: infopanel.right
		anchors.right: right_bar.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
	}
	
	CharacterDialog {
		id: character_dialog
		show_family_tree_button: false
	}
}
