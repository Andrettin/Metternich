import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: character_class_choice_dialog
	title: "Choose a Character Class"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: character_class_button_column_view.y + character_class_button_column_view.height + 8 * scale_factor
	
	property var character: null
	property var potential_classes: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which class shall you advance to?"
		wrapMode: Text.WordWrap
	}
	
	Flickable {
		id: character_class_button_column_view
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.right: parent.right
		height: Math.min(contentHeight, 384 * scale_factor)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Column {
			id: character_class_button_column
			anchors.top: parent.top
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 8 * scale_factor
			
			Repeater {
				model: potential_classes
				
				TextButton {
					id: character_class_button
					text: format_text(character_class ? character_class.name : "None")
					width: character_class_choice_dialog.width - 16 * scale_factor
					
					readonly property var character_class: model.modelData
					
					onClicked: {
						character.game_data.on_character_class_chosen(character_class)
						character_class_choice_dialog.close()
						character_class_choice_dialog.destroy()
					}
				}
			}
		}
	}
}
