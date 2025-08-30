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
	
	Column {
		id: content_column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 16 * scale_factor
		
		Grid {
			id: military_unit_grid
			anchors.horizontalCenter: parent.horizontalCenter
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
						
						readonly property string costs_string: military_unit_type !== null ? costs_to_string(country_game_data.military.get_military_unit_type_commodity_costs_qvariant_list(military_unit_type, 1), undefined, ", ") : ""
						readonly property string stats_string: military_unit_type !== null ? military_unit_type.get_stats_for_country_qstring(metternich.game.player_country) : ""
						
						onClicked: {
						}
						
						onHoveredChanged: {
							if (hovered) {
								status_text = military_unit_type.name
								middle_status_text = format_text("Costs: " + costs_string)
								right_status_text = format_text(stats_string)
							} else {
								status_text = ""
								middle_status_text = ""
								right_status_text = ""
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
		
		TextButton {
			id: ok_button
			anchors.horizontalCenter: parent.horizontalCenter
			text: "OK"
			onClicked: {
				military_unit_recruiment_dialog.close()
			}
		}
	}
}
