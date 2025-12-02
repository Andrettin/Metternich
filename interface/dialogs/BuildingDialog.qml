import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: building_dialog
	title: building ? building.name : ""
	width: Math.max(content_column.width + 8 * scale_factor * 2, 256 * scale_factor)
	height: content_column.y + content_column.height + 8 * scale_factor
	
	property var building_slot: null
	readonly property var building: building_slot ? building_slot.building : null
	readonly property string modifier_string: building_slot ? building_slot.country_modifier_string : 0
	
	UpgradeBuildingButton {
		id: upgrade_building_button
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 16 * scale_factor
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
			visible: building && modifier_string.length > 0 && !building.warehouse
		}
		
		SmallText {
			id: storage_capacity_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Storage Capacity: " + number_string(country_game_data.economy.storage_capacity)
			visible: building && building.warehouse
		}
		
		Grid {
			id: commodity_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 4
			visible: building && building.warehouse
			
			Repeater {
				model: country_game_data.economy.stored_commodities
				
				Item {
					width: 64 * scale_factor
					height: 64 * scale_factor
					visible: !commodity.abstract
					
					readonly property var commodity: model.modelData.key
					readonly property int stored: model.modelData.value
					
					Image {
						id: commodity_icon
						anchors.verticalCenter: parent.verticalCenter
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.horizontalCenterOffset: -8 * scale_factor
						source: "image://icon/" + commodity.icon.identifier
					}
					
					SmallText {
						id: stored_label
						text: number_string(stored)
						anchors.left: commodity_icon.right
						anchors.leftMargin: 4 * scale_factor
						anchors.bottom: commodity_icon.bottom
					}
					
					MouseArea {
						anchors.fill: commodity_icon
						hoverEnabled: true
						
						onEntered: {
							status_text = commodity.name
						}
						
						onExited: {
							status_text = ""
						}
					}
				}
			}
		}
		
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
					visible: country_game_data.can_gain_civilian_unit(civilian_unit_type)
					
					readonly property var civilian_unit_type: model.modelData
					property int civilian_unit_recruitment_count: country_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
					
					IconButton {
						id: civilian_unit_button
						width: 64 * scale_factor + 6 * scale_factor
						height: 64 * scale_factor + 6 * scale_factor
						icon_identifier: civilian_unit_type.icon.identifier
						tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
						
						readonly property string costs_string: civilian_unit_type !== null ? costs_to_string(civilian_unit_type.commodity_costs, undefined, "\n\t", civilian_unit_type.wealth_cost) : ""
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
						
						IconButton {
							id: decrement_button
							anchors.verticalCenter: parent.verticalCenter
							width: 16 * scale_factor
							height: 16 * scale_factor
							icon_identifier: "trade_consulate"
							use_opacity_mask: false
							
							onReleased: {
								if (country_game_data.can_decrease_civilian_unit_recruitment(civilian_unit_type)) {
									country_game_data.decrease_civilian_unit_recruitment(civilian_unit_type, true)
									civilian_unit_recruitment_count = country_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
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
						
						IconButton {
							id: increment_button
							anchors.verticalCenter: parent.verticalCenter
							width: 16 * scale_factor
							height: 16 * scale_factor
							icon_identifier: "trade_consulate"
							use_opacity_mask: false
							
							onReleased: {
								if (country_game_data.can_increase_civilian_unit_recruitment(civilian_unit_type)) {
									country_game_data.increase_civilian_unit_recruitment(civilian_unit_type)
									civilian_unit_recruitment_count = country_game_data.get_civilian_unit_recruitment_count(civilian_unit_type)
								}
							}
						}
					}
				}
			}
		}
		
		Grid {
			id: transporter_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 3
			visible: building && building.recruited_transporter_categories.length > 0
			spacing: 16 * scale_factor
			
			Repeater {
				model: building ? building.recruited_transporter_categories : []
				
				Column {
					spacing: 2 * scale_factor
					visible: transporter_type !== null
					
					readonly property var transporter_category: model.modelData
					readonly property var transporter_type: country_game_data.get_best_transporter_category_type(transporter_category)
					property int transporter_recruitment_count: transporter_type !== null ? country_game_data.get_transporter_recruitment_count(transporter_type) : 0
					
					IconButton {
						id: transporter_button
						width: 64 * scale_factor + 6 * scale_factor
						height: 64 * scale_factor + 6 * scale_factor
						icon_identifier: transporter_type !== null ? transporter_type.icon.identifier : "skull"
						tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
						
						readonly property string costs_string: transporter_type !== null ? costs_to_string(country_game_data.get_transporter_type_commodity_costs_qvariant_list(transporter_type, 1), undefined, "\n\t", country_game_data.get_transporter_type_wealth_cost(transporter_type, 1)) : ""
						readonly property string tooltip_string: costs_string
						
						onClicked: {
						}
						
						onHoveredChanged: {
							if (hovered) {
								status_text = transporter_type.name
							} else {
								status_text = ""
							}
						}
					}
					
					Row {
						anchors.horizontalCenter: parent.horizontalCenter
						
						IconButton {
							id: decrement_button
							anchors.verticalCenter: parent.verticalCenter
							width: 16 * scale_factor
							height: 16 * scale_factor
							icon_identifier: "trade_consulate"
							use_opacity_mask: false
							
							onReleased: {
								if (country_game_data.can_decrease_transporter_recruitment(transporter_type)) {
									country_game_data.decrease_transporter_recruitment(transporter_type, true)
									transporter_recruitment_count = country_game_data.get_transporter_recruitment_count(transporter_type)
								}
							}
						}
						
						SmallText {
							id: transporter_recruiting_count_label
							anchors.verticalCenter: parent.verticalCenter
							text: number_string(transporter_recruitment_count)
							width: 24 * scale_factor
							horizontalAlignment: Text.AlignHCenter
						}
						
						IconButton {
							id: increment_button
							anchors.verticalCenter: parent.verticalCenter
							width: 16 * scale_factor
							height: 16 * scale_factor
							icon_identifier: "trade_consulate"
							use_opacity_mask: false
							
							onReleased: {
								if (country_game_data.can_increase_transporter_recruitment(transporter_type)) {
									country_game_data.increase_transporter_recruitment(transporter_type)
									transporter_recruitment_count = country_game_data.get_transporter_recruitment_count(transporter_type)
								}
							}
						}
					}
				}
			}
		}
		
		TextButton {
			id: ok_button
			anchors.horizontalCenter: parent.horizontalCenter
			text: "OK"
			onClicked: {
				building_dialog.close()
			}
		}
	}
}
