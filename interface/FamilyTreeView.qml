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
		
		Repeater {
			model: family_tree.entries
			
			TreePortraitButton {
				tree_line_visible: false
				
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
	}
	
	readonly property int portrait_button_width: 64 * scale_factor + 2 * scale_factor
	readonly property int portrait_button_height: 64 * scale_factor + 2 * scale_factor
	readonly property int horizontal_padding: 8 * scale_factor
	readonly property int vertical_padding: 32 * scale_factor
	
	readonly property int portrait_button_area_width: portrait_button_width + horizontal_padding * 2
	readonly property int portrait_button_area_height: portrait_button_height + vertical_padding * 2
	
	function center_on_character(center_character) {
		var pixel_x = center_character.tree_x * portrait_button_area_width + (center_character.tree_width - 1) * Math.floor(portrait_button_area_width / 2) + horizontal_padding + Math.floor(portrait_button_area_width / 2) - Math.floor(family_tree.width / 2)
		var pixel_y = center_character.tree_y * portrait_button_area_height + Math.floor(portrait_button_area_height / 2) - Math.floor(family_tree.height / 2)
		
		if (family_tree.contentWidth > family_tree.width) {
			family_tree.contentX = Math.min(Math.max(pixel_x, 0), family_tree.contentWidth - family_tree.width)
		}
		if (family_tree.contentHeight > family_tree.height) {
			family_tree.contentY = Math.min(Math.max(pixel_y, 0), family_tree.contentHeight - family_tree.height)
		}
	}
	
	Component.onCompleted: {
		family_tree_view.center_on_character(family_tree_view.character)
	}
}
