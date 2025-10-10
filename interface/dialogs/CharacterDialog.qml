import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import character_data_model 1.0
import ".."

DialogBase {
	id: character_dialog
	width: 320 * scale_factor
	height: button_column.y + button_column.height + 8 * scale_factor
	title: character ? character.game_data.titled_name : ""
	
	property var character: null
	property var office: null
	readonly property bool is_appointee: office ? metternich.game.player_country.game_data.government.get_appointed_office_holder(office) === character : false
	
	PortraitButton {
		id: character_portrait
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: character ? (character.game_data.portrait.identifier + (is_appointee ? "/grayscale" : "")) : ""
	}
	
	Rectangle {
		id: character_data_tree_view_top_divisor
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: character_portrait.bottom
		anchors.topMargin: 16 * scale_factor
		height: 1 * scale_factor
	}
	
	TreeView {
		id: character_data_tree_view
		anchors.left: parent.left
		anchors.leftMargin: 1 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 1 * scale_factor
		anchors.top: character_data_tree_view_top_divisor.bottom
		height: Math.min(256 * scale_factor, contentHeight)
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		enabled: character_dialog.activeFocus
		model: CharacterDataModel {
			character: character_dialog.character
			
			onCharacterChanged: {
				character_data_tree_view.contentY = 0
			}
		}
		delegate: TreeViewDelegate {
			implicitWidth: character_data_tree_view.width
			font.family: berenika_font.name
			font.pixelSize: 10 * scale_factor
			Material.theme: Material.Dark
			
			onClicked: {
				if (character === metternich.game.player_character) {
					if (model.item) {
						if (model.item.equipped) {
							character.game_data.deequip_item(model.item)
						} else if (character.game_data.can_equip_item(model.item, true)) {
							character.game_data.equip_item(model.item)
						}
					}
				}
			}
			
			onHoveredChanged: {
				var text = ""
				var middle_text = ""
				
				if (model.item) {
					text = model.item.name
					
					if (character === metternich.game.player_character) {
						if (model.item.equipped) {
							middle_text = "Click to de-equip"
						} else if (character.game_data.can_equip_item(model.item, true)) {
							middle_text = "Click to equip"
						}
					}
				}
				
				if (typeof status_text !== 'undefined') {
					if (hovered) {
						status_text = text
						middle_status_text = middle_text
					} else {
						if (status_text === text && middle_status_text === middle_text) {
							status_text = ""
							middle_status_text = ""
						}
					}
				}
			}
		}
	}
	
	Rectangle {
		id: character_data_tree_view_bottom_divisor
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: character_data_tree_view.bottom
		height: 1 * scale_factor
	}
	
	Column {
		id: button_column
		anchors.top: character_data_tree_view_bottom_divisor.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: appoint_button
			text: is_appointee ? "Unappoint" : "Appoint"
			visible: office !== null && office.appointable
			onClicked: {
				if (is_appointee) {
					metternich.game.player_country.game_data.government.set_appointed_office_holder(office, null)
					character_dialog.close()
				} else {
					office_holder_choice_dialog.office = office
					office_holder_choice_dialog.open()
				}
			}
		}
		
		TextButton {
			id: family_tree_button
			text: "Family Tree"
			visible: character !== null && (character.father !== null || character.mother !== null)
			onClicked: {
				family_tree_dialog.character = character
				family_tree_dialog.open()
				family_tree_dialog.receive_focus()
			}
		}
		
		TextButton {
			id: ok_button
			text: "OK"
			onClicked: {
				character_dialog.close()
			}
		}
	}
	
	onClosed: {
		character = null
		office = null
	}
}
