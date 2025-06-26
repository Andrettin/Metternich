import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

PortraitGrid {
	id: idea_portrait_grid
	delegate: PortraitGridItem {
		portrait_identifier: appointed_idea ? appointed_idea.portrait.identifier + "/grayscale" : (idea ? idea.portrait.identifier : "no_character")
		
		readonly property var idea_slot: model.modelData
		readonly property var idea: country_game_data.ideas.length > 0 ? country_game_data.get_idea(idea_slot) : null //the check here is for the sake of property binding
		readonly property var appointed_idea: country_game_data.appointed_ideas.length > 0 ? country_game_data.get_appointed_idea(idea_slot) : null //the check here is for the sake of property binding
		
		onClicked: {
			if (idea !== null || appointed_idea !== null) {
				idea_dialog.idea_slot = idea_slot
				idea_dialog.idea = appointed_idea ? appointed_idea : idea
				idea_dialog.open()
			} else {
				idea_choice_dialog.idea_slot = idea_slot
				idea_choice_dialog.open()
			}
		}
		
		onEntered: {
			status_text = appointed_idea ? "Appointing " + appointed_idea.get_cultural_name_qstring(country.culture) : (idea ? idea.get_cultural_name_qstring(country.culture) : idea_slot.name + " Slot")
		}
		
		onExited: {
			status_text = ""
		}
	}
}
