import QtQuick
import QtQuick.Controls
import ".."

ModifierDialog {
	id: character_dialog
	title: character ? character.game_data.titled_name : ""
	portrait_identifier: character ? (character.game_data.portrait.identifier + (is_appointee ? "/grayscale" : "")) : ""
	date_string: character ? ("Lived: " + date_year_range_string(character.birth_date, character.death_date)) : ""
	description: character ? character.description : ""
	ok_button_top_anchor: office ? appoint_button.bottom : default_ok_button_top_anchor
	ok_button_top_margin: office ? 8 * scale_factor : 16 * scale_factor
	
	property var character: null
	property var office: null
	readonly property bool is_appointee: office ? metternich.game.player_country.game_data.get_appointed_office_holder(office) === character : false
	
	TextButton {
		id: appoint_button
		anchors.top: default_ok_button_top_anchor
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: is_appointee ? "Unappoint" : "Appoint"
		visible: office !== null && office.appointable
		onClicked: {
			if (is_appointee) {
				metternich.game.player_country.game_data.set_appointed_office_holder(office, null)
				character_dialog.close()
			} else {
				office_holder_choice_dialog.office = office
				office_holder_choice_dialog.open()
			}
		}
	}
}
