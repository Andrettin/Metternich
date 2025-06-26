import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: advisor_choice_dialog
	title: "Choose Advisor"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: advisor_button_column.y + advisor_button_column.height + 8 * scale_factor
	
	property var potential_advisors: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which advisor shall we recruit next?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: advisor_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: potential_advisors
			
			TextButton {
				id: advisor_button
				text: format_text(advisor.full_name)
				width: advisor_choice_dialog.width - 16 * scale_factor
				tooltip: format_text(small_text(advisor.game_data.get_advisor_effects_string(metternich.game.player_country)))
				
				readonly property var advisor: model.modelData
				
				onClicked: {
					metternich.game.player_country.game_data.next_advisor = advisor
					advisor_choice_dialog.close()
				}
			}
		}
	}
}
