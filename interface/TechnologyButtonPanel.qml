import QtQuick
import QtQuick.Controls

Rectangle {
	id: button_panel
	color: interface_background_color
	width: technology_view_mode !== TechnologyView.Mode.TechTree ? (64 * scale_factor + 48 * scale_factor) : 16 * scale_factor
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
	
	Column {
		id: category_button_column
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.horizontalCenterOffset: 24 * scale_factor
		spacing: 4 * scale_factor
		
		Repeater {
			model: metternich.get_technology_categories()
			
			IconButton {
				id: category_button
				icon_identifier: button_category.icon.identifier
				highlighted: technology_view_category === button_category
				visible: technology_view_mode !== TechnologyView.Mode.TechTree
						
				readonly property var button_category: model.modelData
				
				onReleased: {
					if (technology_view_category !== button_category) {
						technology_view_category = button_category
						technology_view_subcategory = null
					}
				}
				
				onHoveredChanged: {
					if (hovered) {
						status_text = button_category.name
					} else {
						status_text = ""
					}
				}
			}
		}
	}
	
	Column {
		id: subcategory_button_column
		anchors.top: category_button_column.top
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.horizontalCenterOffset: -24 * scale_factor
		spacing: 4 * scale_factor
		
		Repeater {
			model: technology_view_category ? get_category_subcategories(technology_view_category) : []
			
			IconButton {
				id: category_button
				icon_identifier: button_subcategory.icon.identifier
				highlighted: technology_view_subcategory === button_subcategory
				visible: technology_view_mode !== TechnologyView.Mode.TechTree
						
				readonly property var button_subcategory: model.modelData
				
				onClicked: {
					technology_view_subcategory = button_subcategory
				}
				
				onHoveredChanged: {
					if (hovered) {
						status_text = button_subcategory.name
					} else {
						status_text = ""
					}
				}
			}
		}
	}
	
	IconButton {
		id: no_category_button
		anchors.top: category_button_column.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.horizontalCenterOffset: 24 * scale_factor
		icon_identifier: "university"
		highlighted: technology_view_category === null
		visible: technology_view_mode !== TechnologyView.Mode.TechTree
		
		onClicked: {
			technology_view_category = null
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
		id: no_subcategory_button
		anchors.top: subcategory_button_column.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.horizontalCenterOffset: -24 * scale_factor
		icon_identifier: "university"
		highlighted: technology_view_subcategory === null
		visible: technology_view_mode !== TechnologyView.Mode.TechTree && technology_view_category !== null
		
		onClicked: {
			technology_view_subcategory = null
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Show All"
			} else {
				status_text = ""
			}
		}
	}
	
	function get_category_subcategories(category) {
		var subcategories = []
		
		for (var subcategory of category.subcategories) {
			var has_technology = false
			for (var technology of subcategory.technologies) {
				if (technology.is_available_for_country(metternich.game.player_country)) {
					has_technology = true
					break
				}
			}
			if (has_technology) {
				subcategories.push(subcategory)
			}
		}
		
		return subcategories
	}
}
