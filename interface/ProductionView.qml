import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: production_view
	
	readonly property var domain: politics_view.country
	readonly property var domain_game_data: domain ? domain.game_data : null
	readonly property var domain_economy: domain_game_data ? domain_game_data.economy : null
	
	SmallText {
		id: storage_capacity_label
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Storage Capacity: " + number_string(domain_economy.storage_capacity)
	}
	
	Grid {
		id: commodity_grid
		anchors.top: storage_capacity_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 4
		spacing: 16 * scale_factor
		
		Repeater {
			model: domain_economy.available_commodities
			
			Item {
				width: 64 * scale_factor
				height: 64 * scale_factor
				visible: !commodity.abstract && (commodity.wealth_value === 0 || output > 0)
				
				readonly property var commodity: model.modelData
				readonly property int stored: domain_economy.get_stored_commodity(commodity)
				readonly property int output: domain_economy.get_commodity_output_int(commodity)
				
				Image {
					id: commodity_icon
					anchors.verticalCenter: parent.verticalCenter
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.horizontalCenterOffset: -8 * scale_factor
					source: "image://icon/" + commodity.icon.identifier
				}
				
				SmallText {
					id: stored_label
					text: commodity.value_to_qstring(stored)
					anchors.left: commodity_icon.right
					anchors.leftMargin: 4 * scale_factor
					anchors.bottom: commodity_icon.bottom
					visible: commodity.wealth_value === 0
				}
				
				MouseArea {
					anchors.fill: commodity_icon
					hoverEnabled: true
					
					onEntered: {
						status_text = commodity.name
						if (output > 0) {
							middle_status_text = "Output: " + commodity.value_to_qstring(output)
							if (commodity.wealth_value !== 0) {
								middle_status_text += " (" + metternich.defines.wealth_commodity.value_to_qstring(commodity.wealth_value * output) + ")"
							}
						}
						right_status_text = commodity.get_units_tooltip()
					}
					
					onExited: {
						status_text = ""
						middle_status_text = ""
						right_status_text = ""
					}
				}
			}
		}
	}
}
