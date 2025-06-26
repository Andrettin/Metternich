import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import ".."

DialogBase {
	id: dialog
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: cancel_button.y + cancel_button.height + 8 * scale_factor
	
	property var idea_slot: null
	readonly property var ideas: idea_slot ? metternich.game.player_country.game_data.get_appointable_ideas_qvariant_list(idea_slot) : []
	property string text: ""
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: dialog.text
		wrapMode: Text.WordWrap
	}
	
	Column {
		id: idea_button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		visible: ideas.length > 0
		
		Repeater {
			model: ideas
			
			TextButton {
				id: idea_button
				text: format_text(idea.get_cultural_name_qstring(metternich.game.player_country.culture))
				width: dialog.width - 16 * scale_factor
				tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
				
				readonly property var idea: model.modelData
				readonly property string costs_string: costs_to_string(metternich.game.player_country.game_data.get_idea_commodity_costs_qvariant_list(idea))
				readonly property string modifier_string: idea.get_modifier_qstring(metternich.game.player_country)
				readonly property string tooltip_string: costs_string + (costs_string.length > 0 && modifier_string.length > 0 ? "\n\n" : "") + modifier_string
				
				onClicked: {
					if (metternich.game.player_country.game_data.can_appoint_idea(idea_slot, idea)) {
						metternich.game.player_country.game_data.set_appointed_idea(idea_slot, idea)
						dialog.close()
					} else {
						add_notification("Costs", metternich.game.player_country.game_data.interior_minister_portrait, "Your Excellency, we unfortunately cannot pay the costs of selecting a new " + idea_slot.name.toLowerCase() + ".", dialog.parent)
					}
				}
			}
		}
	}
	
	TextButton {
		id: cancel_button
		anchors.top: idea_button_column.visible ? idea_button_column.bottom : text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Cancel"
		onClicked: {
			dialog.close()
		}
	}
}
