import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import "./dialogs"

Item {
	id: research_organizations_view
	
	readonly property var research_organization_slots: country_game_data.available_research_organization_slots
	
	IdeaSlotGrid {
		id: research_organization_portrait_grid
		anchors.top: parent.top
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: research_organization_slots
	}
	
	IdeaDialog {
		id: idea_dialog
	}
	
	IdeaChoiceDialog {
		id: idea_choice_dialog
		title: "Choose Research Organization"
		text: "Which research organization shall we establish a contract with?"
	}
}
