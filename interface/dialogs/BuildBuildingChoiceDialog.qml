import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: build_building_choice_dialog
	title: "Build Structure"
	width: building_button_column.width + 8 * scale_factor * 2
	height: cancel_button.y + cancel_button.height + 8 * scale_factor
	
	property var civilian_unit: null
	property var potential_traits: []
	
	SmallText {
		id: text_label
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		text: "Which building do you wish to build?"
		wrapMode: Text.WordWrap
		horizontalAlignment: Text.AlignHCenter
	}
	
	Flickable {
		id: building_button_column_view
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: Math.min(contentHeight, 384 * scale_factor)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Column {
			id: building_button_column
			anchors.top: parent.top
			anchors.left: parent.left
			spacing: 4 * scale_factor
			
			Repeater {
				model: civilian_unit !== null ? civilian_unit.buildable_buildings : []
				
				Column {
					id: site_building_button_column
					anchors.left: parent.left
					spacing: 4 * scale_factor
					
					readonly property var site: model.modelData.key
					readonly property var buildable_buildings: model.modelData.value
					
					Repeater {
						model: buildable_buildings
						
						Row {
							anchors.left: parent.left
							spacing: 8 * scale_factor
							
							readonly property var building: model.modelData
							
							IconButton {
								id: building_button
								anchors.verticalCenter: parent.verticalCenter
								icon_identifier: building.icon.identifier
								
								onClicked: {
									civilian_unit.build_building(building, site)
									build_building_choice_dialog.close()
								}
								
								onHoveredChanged: {
									if (hovered) {
										status_text = "Build " + building.name + " in " + site.game_data.current_cultural_name
										middle_status_text = building.get_commodity_costs_string_for_site(site)
										right_status_text = building.get_effects_string(site)
									} else {
										status_text = ""
										middle_status_text = ""
										right_status_text = ""
									}
								}
							}
							
							SmallText {
								id: building_label
								text: building.name + " (" + site.game_data.current_cultural_name + ")"
								anchors.verticalCenter: parent.verticalCenter
							}
						}
					}
				}
			}
		}
	}
	
	TextButton {
		id: cancel_button
		anchors.top: building_button_column_view.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Cancel"
		onClicked: {
			build_building_choice_dialog.close()
		}
	}
	
	onClosed: {
		if (civilian_unit === selected_civilian_unit) {
			go_to_next_civilian_unit(true)
		}
		
		civilian_unit = null
	}
}
