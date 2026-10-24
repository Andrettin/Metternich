import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: building_dialog
	title: wonder ? wonder.name : (building ? building.name : "")
	width: Math.max(content_column.width + 8 * scale_factor * 2, 256 * scale_factor)
	height: content_column.y + content_column.height + 8 * scale_factor
	
	property var building_slot: null
	readonly property var building: building_slot ? building_slot.building : null
	readonly property var wonder: building_slot ? building_slot.wonder : null
	readonly property string modifier_string: building_slot ? building_slot.modifier_string : 0
	
	UpgradeBuildingButton {
		id: upgrade_building_button
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
	}
	
	Column {
		id: content_column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 16 * scale_factor
		
		SmallText {
			id: modifier_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: format_text(modifier_string)
			visible: building && modifier_string.length > 0
		}
		
		ItemShopGrid {
			id: item_shop_grid
			item_slots: building_slot ? building_slot.item_slots : []
			visible: building_slot && building_slot.item_slots.length > 0
		}
		
		/*
		Grid {
			id: civilian_unit_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 3
			visible: building && building.recruited_civilian_unit_types.length > 0
			spacing: 16 * scale_factor
			
			Repeater {
				model: building ? building.recruited_civilian_unit_types : []
				
				Column {
					spacing: 2 * scale_factor
					visible: domain_game_data.can_gain_civilian_unit(civilian_unit_type)
					
					readonly property var civilian_unit_type: model.modelData
					property int civilian_unit_recruitment_count: domain_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
					
					IconButton {
						id: civilian_unit_button
						width: 64 * scale_factor + 6 * scale_factor
						height: 64 * scale_factor + 6 * scale_factor
						icon_identifier: civilian_unit_type.icon.identifier
						tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
						
						readonly property string costs_string: civilian_unit_type !== null ? costs_to_string(civilian_unit_type.commodity_costs, undefined, "\n\t") : ""
						readonly property string tooltip_string: costs_string
						
						onClicked: {
						}
						
						onHoveredChanged: {
							if (hovered) {
								status_text = civilian_unit_type.name
							} else {
								status_text = ""
							}
						}
					}
					
					Row {
						anchors.horizontalCenter: parent.horizontalCenter
						
						TinyIconButton {
							id: decrement_button
							anchors.verticalCenter: parent.verticalCenter
							icon_identifier: "arrow_left"
							
							onReleased: {
								if (domain_game_data.can_decrease_civilian_unit_recruitment(civilian_unit_type)) {
									domain_game_data.decrease_civilian_unit_recruitment(civilian_unit_type, true)
									civilian_unit_recruitment_count = domain_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
								}
							}
						}
						
						SmallText {
							id: civilian_unit_recruiting_count_label
							anchors.verticalCenter: parent.verticalCenter
							text: number_string(civilian_unit_recruitment_count)
							width: 24 * scale_factor
							horizontalAlignment: Text.AlignHCenter
						}
						
						TinyIconButton {
							id: increment_button
							anchors.verticalCenter: parent.verticalCenter
							icon_identifier: "arrow_right"
							
							onReleased: {
								if (domain_game_data.can_increase_civilian_unit_recruitment(civilian_unit_type)) {
									domain_game_data.increase_civilian_unit_recruitment(civilian_unit_type)
									civilian_unit_recruitment_count = domain_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
								}
							}
						}
					}
				}
			}
		}
		*/
		
		TextButton {
			id: ok_button
			anchors.horizontalCenter: parent.horizontalCenter
			text: "OK"
			onClicked: {
				building_dialog.building_slot = null
				building_dialog.close()
			}
		}
	}
}
