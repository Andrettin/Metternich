import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: research_choice_dialog
	title: free_technology ? "Choose Free Technology" : "Choose Research Objective"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: technology_button_column.y + technology_button_column.height + 8 * scale_factor
	
	property var potential_technologies: []
	property bool free_technology: false
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: free_technology ? "Which technology shall we acquire?" : "Which technology shall we research next?"
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: technology_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		Repeater {
			model: potential_technologies
			
			TextButton {
				id: technology_button
				text: format_text(technology.name)
				width: research_choice_dialog.width - 16 * scale_factor
				tooltip: effects_string.length > 0 ? format_text(small_text(effects_string)) : ""
				
				readonly property var technology: model.modelData
				readonly property string effects_string: technology.get_effects_string(metternich.game.player_country)
				
				onClicked: {
					research_choice_dialog.close()
					if (free_technology) {
						metternich.game.player_country.game_data.gain_free_technology(technology)
					}
				}
			}
		}
	}
}
