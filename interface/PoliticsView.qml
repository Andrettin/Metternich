import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: politics_view
	
	enum Mode {
		Advisors,
		Government,
		Religion,
		ResearchOrganizations
	}
	
	property var country: null
	readonly property var country_game_data: country ? country.game_data : null
	readonly property var ruler: country_game_data ? country_game_data.ruler : null
	property string status_text: ""
	property string middle_status_text: ""
	
	TiledBackground {
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	AdvisorsView {
		id: advisors_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: politics_view_mode === PoliticsView.Mode.Advisors
	}
	
	GovernmentView {
		id: government_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: politics_view_mode === PoliticsView.Mode.Government
	}
	
	ReligionView {
		id: religion_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: politics_view_mode === PoliticsView.Mode.Religion
	}
	
	ResearchOrganizationsView {
		id: research_organizations_view
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
		visible: politics_view_mode === PoliticsView.Mode.ResearchOrganizations
	}
	
	PoliticsButtonPanel {
		id: button_panel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	AdvisorsInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: infopanel.right
		anchors.right: button_panel.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: infopanel.right
		anchors.right: button_panel.left
	}
	
	CharacterDialog {
		id: character_dialog
	}
}
