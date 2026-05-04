import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: military_unit_recruiment_dialog
	title: "Recruit Military Units"
	width: Math.max(content_column.width + 8 * scale_factor * 2, 256 * scale_factor)
	height: content_column.y + content_column.height + 8 * scale_factor
	
	property var recruitable_military_unit_categories: selected_province ? selected_province.game_data.recruitable_military_unit_categories : []
	readonly property var country_game_data: metternich.game.player_country.game_data
	property var selected_military_unit_type: null
	
	Column {
		id: content_column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 16 * scale_factor
		
		Row {
			id: content_row
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 16 * scale_factor
			
			Column {
				id: military_unit_info_column
				anchors.top: parent.top
				spacing: 16 * scale_factor
				width: 48 * scale_factor * 3 + 16 * scale_factor * 2
				height: military_unit_grid.height
				
				SmallText {
					id: military_unit_type_name
					anchors.horizontalCenter: parent.horizontalCenter
					text: selected_military_unit_type ? selected_military_unit_type.name : ""
				}
				
				Row {
					id: military_unit_type_stats_row
					spacing: 16 * scale_factor
					
					Column {
						id: military_unit_type_stats_column
						spacing: 4 * scale_factor
						
						Repeater {
							model: selected_military_unit_type ? selected_military_unit_type.get_stats_for_domain_qvariant_list(metternich.game.player_country) : []
							
							Row {
								id: military_unit_stat_row
								width: military_unit_info_column.width - selected_military_unit_type_icon.width - military_unit_type_stats_row.spacing
								readonly property string stat_name: model.modelData.key
								readonly property string stat_value: model.modelData.value
								
								SmallText {
									id: military_unit_stat_name
									anchors.verticalCenter: parent.verticalCenter
									text: stat_name + ":"
									horizontalAlignment: Text.AlignLeft
								}
								
								SmallText {
									id: military_unit_stat_value
									anchors.verticalCenter: parent.verticalCenter
									text: stat_value
									horizontalAlignment: Text.AlignRight
									width: military_unit_stat_row.width - military_unit_stat_name.width
								}
							}
						}
					}
					
					CustomIconImage {
						id: selected_military_unit_type_icon
						anchors.top: parent.top
						anchors.topMargin: 8 * scale_factor
						name: selected_military_unit_type ? selected_military_unit_type.name : ""
						icon_identifier: selected_military_unit_type ? selected_military_unit_type.icon.identifier : ""
					}
				}
				
				Item {
					id: military_unit_info_column_spacing
					width: military_unit_type_stats_column.width
					height: military_unit_grid.height - y - military_unit_type_cost_column.height - military_unit_type_available_commodities_column.height - military_unit_info_column.spacing * 2
				}
				
				Column {
					id: military_unit_type_cost_column
					spacing: 4 * scale_factor
					
					SmallText {
						id: military_unit_type_cost_label
						text: "Cost:"
					}
					
					Row {
						id: military_unit_type_cost_row
						spacing: 16 * scale_factor
						
						Repeater {
							model: selected_military_unit_type ? country_game_data.military.get_military_unit_type_commodity_costs_qvariant_list(selected_military_unit_type, 1) : []
							
							Column {
								id: cost_commodity_column
								width: 48 * scale_factor
							
								readonly property var commodity: model.modelData.key
								readonly property int cost: model.modelData.value
								
								CustomIconImage {
									id: cost_commodity_icon
									anchors.horizontalCenter: parent.horizontalCenter
									name: commodity.name
									icon_identifier: commodity.icon.identifier
								}
								
								SmallText {
									id: commodity_cost_label
									anchors.right: parent.right
									text: commodity.value_to_qstring(cost)
								}
							}
						}
					}
				}
				
				Column {
					id: military_unit_type_available_commodities_column
					spacing: 4 * scale_factor
					
					SmallText {
						id: military_unit_type_available_commodities_label
						text: "Available:"
					}
					
					Row {
						id: military_unit_type_available_commodities_row
						spacing: military_unit_type_cost_row.spacing
						
						Repeater {
							model: selected_military_unit_type ? country_game_data.military.get_military_unit_type_commodity_costs_qvariant_list(selected_military_unit_type, 1) : []
							
							Column {
								id: available_commodity_column
								width: 48 * scale_factor
							
								readonly property var commodity: model.modelData.key
								readonly property int stored: country_game_data.economy.stored_commodities.length > 0 ? country_game_data.economy.get_stored_commodity(commodity) : 0
								
								CustomIconImage {
									id: available_commodity_icon
									anchors.horizontalCenter: parent.horizontalCenter
									name: commodity.name
									icon_identifier: commodity.icon.identifier
								}
								
								SmallText {
									id: available_commodity_label
									anchors.right: parent.right
									text: commodity.value_to_qstring(stored)
								}
							}
						}
					}
				}
			}
			
			Grid {
				id: military_unit_grid
				anchors.top: parent.top
				columns: 3
				visible: recruitable_military_unit_categories.length > 0
				spacing: 16 * scale_factor
				
				Repeater {
					model: recruitable_military_unit_categories
					
					Column {
						spacing: 2 * scale_factor
						visible: military_unit_type !== null
						
						readonly property var military_unit_category: model.modelData
						readonly property var military_unit_type: country_game_data.military.get_best_military_unit_category_type(military_unit_category)
						readonly property int military_unit_recruitment_count: military_unit_type !== null && selected_province.game_data.military_unit_recruitment_counts.length > 0 ? selected_province.game_data.get_military_unit_recruitment_count(military_unit_type) : 0
						
						IconButton {
							id: military_unit_button
							width: 64 * scale_factor + 6 * scale_factor
							height: 64 * scale_factor + 6 * scale_factor
							icon_identifier: military_unit_type !== null ? military_unit_type.icon.identifier : "skull"
							highlighted: selected_military_unit_type === military_unit_type
							
							onClicked: {
								selected_military_unit_type = military_unit_type
							}
							
							onHoveredChanged: {
								if (hovered) {
									status_text = military_unit_type.name
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
									if (selected_province.game_data.can_decrease_military_unit_recruitment(military_unit_type)) {
										selected_province.game_data.decrease_military_unit_recruitment(military_unit_type, true)
									}
								}
							}
							
							SmallText {
								id: military_unit_recruiting_count_label
								anchors.verticalCenter: parent.verticalCenter
								text: number_string(military_unit_recruitment_count)
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
									if (selected_province.game_data.can_increase_military_unit_recruitment(military_unit_type)) {
										selected_province.game_data.increase_military_unit_recruitment(military_unit_type)
									}
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
				military_unit_recruiment_dialog.close()
			}
		}
	}
	
	onOpened: {
		for (var military_unit_category of recruitable_military_unit_categories) {
			var military_unit_type = country_game_data.military.get_best_military_unit_category_type(military_unit_category)
			if (military_unit_type !== null) {
				selected_military_unit_type = military_unit_type
				break
			}
		}
	}
	
	onClosed: {
		selected_military_unit_type = null
	}
}
