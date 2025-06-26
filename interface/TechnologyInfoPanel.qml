import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
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
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	Column {
		id: commodity_counter_column
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		spacing: metternich.get_research_commodities().length >= 5 ? (4 * scale_factor) : (8 * scale_factor)
		
		Repeater {
			model: metternich.get_research_commodities()
			
			IndustryCounter {
				name: commodity.name
				icon_identifier: commodity.icon.identifier
				count: country_game_data.stored_commodities.length > 0 ? country_game_data.get_stored_commodity(commodity) : 0
				visible: commodity.enabled
				
				readonly property var commodity: model.modelData
			}
		}
	}
	
	IconButton {
		id: researched_mode_button
		anchors.top: commodity_counter_column.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "architecture"
		highlighted: technology_view_mode === TechnologyView.Mode.Researched
		
		onClicked: {
			technology_view_mode = TechnologyView.Mode.Researched
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Researched Technologies"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: available_mode_button
		anchors.top: researched_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "research"
		highlighted: technology_view_mode === TechnologyView.Mode.Available
		
		onClicked: {
			technology_view_mode = TechnologyView.Mode.Available
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Available Technologies"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: future_mode_button
		anchors.top: available_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "philosophy"
		highlighted: technology_view_mode === TechnologyView.Mode.Future
		
		onClicked: {
			technology_view_mode = TechnologyView.Mode.Future
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Future Technologies"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: show_all_mode_button
		anchors.top: future_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "university"
		highlighted: technology_view_mode === TechnologyView.Mode.ShowAll
		
		onClicked: {
			technology_view_mode = TechnologyView.Mode.ShowAll
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Show All"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: tech_tree_mode_button
		anchors.top: show_all_mode_button.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		icon_identifier: "cog"
		highlighted: technology_view_mode === TechnologyView.Mode.TechTree
		
		onClicked: {
			technology_view_mode = TechnologyView.Mode.TechTree
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Technology Tree"
			} else {
				status_text = ""
			}
		}
	}
	
	TextButton {
		id: back_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Back")
		width: 48 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
}
