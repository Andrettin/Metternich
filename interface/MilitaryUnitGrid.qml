import QtQuick
import QtQuick.Controls

Grid {
	id: military_unit_grid
	columns: 2
	
	Repeater {
		model: (selected_province !== null && selected_garrison) ? selected_province.game_data.military_unit_category_counts : []
		
		Item {
			width: 84 * scale_factor
			height: 72 * scale_factor
			
			readonly property var military_unit_category: model.modelData.key
			readonly property int military_unit_count: model.modelData.value
			readonly property int country_military_unit_count: selected_province.game_data.get_country_military_unit_category_count(military_unit_category, metternich.game.player_country)
			
			Image {
				id: military_unit_icon
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: parent.left
				anchors.leftMargin: 4 * scale_factor + (64 * scale_factor - military_unit_icon.width) / 2
				source: "image://icon/" + selected_province.game_data.get_military_unit_category_icon(military_unit_category).identifier
			}
			
			MouseArea {
				anchors.fill: military_unit_icon
				hoverEnabled: true
				
				onEntered: {
					status_text = selected_province.game_data.get_military_unit_category_name(military_unit_category)
				}
				
				onExited: {
					status_text = ""
				}
			}
			
			Image {
				id: military_unit_up_arrow_icon
				anchors.right: parent.right
				anchors.rightMargin: 4 * scale_factor
				anchors.bottom: military_unit_selected_count_label.top
				anchors.bottomMargin: 4 * scale_factor
				source: "image://icon/arrow_up"
				
				MouseArea {
					anchors.fill: parent
					
					onReleased: {
						metternich.change_selected_military_unit_category_count(military_unit_category, 1, selected_province)
						military_unit_selected_count_label.text = number_string(metternich.get_selected_military_unit_category_count(military_unit_category))
					}
				}
			}
			
			SmallText {
				id: military_unit_selected_count_label
				text: number_string(metternich.get_selected_military_unit_category_count(military_unit_category))
				anchors.right: parent.right
				anchors.rightMargin: 4 * scale_factor
				anchors.bottom: military_unit_down_arrow_icon.top
				anchors.bottomMargin: 4 * scale_factor
			}
			
			Image {
				id: military_unit_down_arrow_icon
				anchors.right: parent.right
				anchors.rightMargin: 4 * scale_factor
				anchors.bottom: military_unit_count_label.top
				anchors.bottomMargin: 4 * scale_factor
				source: "image://icon/arrow_down"
				
				MouseArea {
					anchors.fill: parent
					
					onReleased: {
						metternich.change_selected_military_unit_category_count(military_unit_category, -1, selected_province)
						military_unit_selected_count_label.text = number_string(metternich.get_selected_military_unit_category_count(military_unit_category))
					}
				}
			}
			
			SmallText {
				id: military_unit_count_label
				text: military_unit_count === country_military_unit_count ? number_string(military_unit_count) : (number_string(country_military_unit_count) + " (" + number_string(military_unit_count) + ")")
				anchors.bottom: parent.bottom
				anchors.right: parent.right
				anchors.rightMargin: 4 * scale_factor
			}
		}
	}
}
