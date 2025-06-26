import QtQuick
import QtQuick.Controls
import ".."

ModifierDialog {
	id: dialog
	title: idea ? idea.get_cultural_name_qstring(metternich.game.player_country.culture) : ""
	portrait_identifier: idea ? (idea.portrait.identifier + (is_appointee ? "/grayscale" : "")) : ""
	modifier_string: idea ? idea.get_modifier_qstring(country) : ""
	ok_button_top_anchor: appoint_button.bottom
	ok_button_top_margin: 8 * scale_factor
	
	property var idea: null
	property var idea_slot: null
	readonly property bool is_appointee: idea_slot ? metternich.game.player_country.game_data.get_appointed_idea(idea_slot) === idea : false
	
	TextButton {
		id: appoint_button
		anchors.top: modifier_text_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: is_appointee ? "Unappoint" : "Appoint"
		onClicked: {
			if (is_appointee) {
				metternich.game.player_country.game_data.set_appointed_idea(idea_slot, null)
				dialog.close()
			} else {
				idea_choice_dialog.idea_slot = idea_slot
				idea_choice_dialog.open()
			}
		}
	}
}
