import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

DialogBase {
	id: family_tree_dialog
	width: 320 * scale_factor
	height: button_column.y + button_column.height + 8 * scale_factor
	title: character ? ((character.dynasty ? (character.dynasty.name + " ") : "") + "Family Tree") : ""
	
	property var character: null
	
	Rectangle {
		id: family_tree_view_top_divisor
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		height: 1 * scale_factor
	}
	
	FamilyTreeTreeView {
		id: family_tree_view
		anchors.left: parent.left
		anchors.leftMargin: 1 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 1 * scale_factor
		anchors.top: family_tree_view_top_divisor.bottom
		character: family_tree_dialog.character
	}
	
	Rectangle {
		id: family_tree_view_bottom_divisor
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: family_tree_view.bottom
		height: 1 * scale_factor
	}
	
	Column {
		id: button_column
		anchors.top: family_tree_view_bottom_divisor.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: ok_button
			text: "OK"
			onClicked: {
				family_tree_dialog.close()
			}
		}
	}
	
	onOpened: {
		if (character !== null) {
			family_tree_view.expand_to_character()
		}
	}
	
	onClosed: {
		character = null
	}
}
