import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: trait_choice_dialog
	title: trait_type ? ("Choose " + trait_type.name) : ""
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: trait_button_column_view.y + trait_button_column_view.height + 8 * scale_factor
	
	property var character: null
	property var trait_type: null
	property var potential_traits: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: trait_type ? ("Which " + trait_type.name.toLowerCase() + " shall you acquire?") : ""
		wrapMode: Text.WordWrap
	}
	
	Flickable {
		id: trait_button_column_view
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
			id: trait_button_column
			anchors.top: parent.top
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 8 * scale_factor
			
			Repeater {
				model: potential_traits
				
				TextButton {
					id: trait_button
					text: format_text(trait ? trait.name : "None")
					width: trait_choice_dialog.width - 16 * scale_factor
					tooltip: trait ? format_text(small_text(trait.modifier_string)) : ""
					
					readonly property var trait: model.modelData
					
					onClicked: {
						character.game_data.on_trait_chosen(trait, trait_type)
						trait_choice_dialog.close()
						trait_choice_dialog.destroy()
					}
				}
			}
		}
	}
}
