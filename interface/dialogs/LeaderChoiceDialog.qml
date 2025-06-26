import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: leader_choice_dialog
	title: "Choose Leader"
	width: Math.max(content_width, default_width)
	height: content_height
	
	readonly property int max_button_width: calculate_max_button_width(leader_button_column) + 8 * scale_factor * 2
	readonly property int content_width: max_button_width
	readonly property int content_height: leader_button_column.y + leader_button_column.height + 8 * scale_factor
	
	property var potential_leaders: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which leader shall we recruit next?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: leader_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: potential_leaders
			
			TextButton {
				id: leader_button
				text: format_text(leader.full_name + " (" + leader.leader_type_name + ")")
				width: leader_choice_dialog.width - 16 * scale_factor
				
				readonly property var leader: model.modelData
				
				onClicked: {
					metternich.game.player_country.game_data.next_leader = leader
					leader_choice_dialog.close()
				}
			}
		}
	}
}
