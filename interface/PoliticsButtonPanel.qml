import QtQuick
import QtQuick.Controls

Rectangle {
	id: button_panel
	color: interface_background_color
	width: 64 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.left: parent.left
		width: 1 * scale_factor
	}
	
	IconButton {
		id: advisors_button
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "rifle_infantry_light_small"
		highlighted: politics_view_mode === PoliticsView.Mode.Advisors
		
		onClicked: {
			politics_view_mode = PoliticsView.Mode.Advisors
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Advisors"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: government_button
		anchors.top: advisors_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "flag"
		highlighted: politics_view_mode === PoliticsView.Mode.Government
		
		onClicked: {
			politics_view_mode = PoliticsView.Mode.Government
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Government"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: religion_button
		anchors.top: government_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "wooden_cross"
		highlighted: politics_view_mode === PoliticsView.Mode.Religion
		
		onClicked: {
			politics_view_mode = PoliticsView.Mode.Religion
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Religion"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: research_organizations_button
		anchors.top: religion_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "cog"
		highlighted: politics_view_mode === PoliticsView.Mode.ResearchOrganizations
		visible: country_game_data.available_research_organization_slots.length > 0
		
		onClicked: {
			politics_view_mode = PoliticsView.Mode.ResearchOrganizations
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Research Organizations"
			} else {
				status_text = ""
			}
		}
	}
}
