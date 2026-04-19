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
		id: storable_commodity_grid
		anchors.top: storage_capacity_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 4
		spacing: 16 * scale_factor
		
		Repeater {
			model: domain_economy.available_commodities
			
			Column {
				spacing: 4 * scale_factor
				width: 96 * scale_factor
				visible: !commodity.abstract && commodity.wealth_value === 0
				
				readonly property var commodity: model.modelData
				readonly property int stored: domain_economy.get_stored_commodity(commodity)
				readonly property int output: domain_economy.get_commodity_output_int(commodity)
				
				Row {
					anchors.horizontalCenter: parent.horizontalCenter
					height: commodity_icon.height
					spacing: 4 * scale_factor
					
					Image {
						id: commodity_icon
						anchors.verticalCenter: parent.verticalCenter
						source: "image://icon/" + commodity.icon.identifier
					
						MouseArea {
							anchors.fill: parent
							hoverEnabled: true
							
							onEntered: {
								status_text = commodity.name
								if (output > 0) {
									middle_status_text = "Output: " + commodity.value_to_qstring(output)
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
					
					SmallText {
						id: stored_label
						text: commodity.value_to_qstring(stored)
						anchors.bottom: parent.bottom
					}
				}
				
				CustomSlider {
					id: min_storage_slider
					anchors.left: parent.left
					anchors.right: parent.right
					value: domain_game_data.economy.min_commodity_storages.length > 0 ? domain_game_data.economy.get_min_commodity_storage(commodity) : 0
					max_value: domain_game_data.economy.storage_capacity
					
					onDecremented: {
						if (domain_game_data.economy.get_min_commodity_storage(commodity) === min_storage_slider.min_value) {
							return
						}
						
						domain_game_data.economy.change_min_commodity_storage(commodity, -1)
					}
					
					onIncremented: {
						if (domain_game_data.economy.get_min_commodity_storage(commodity) === min_storage_slider.max_value) {
							return
						}
						
						domain_game_data.economy.change_min_commodity_storage(commodity, 1)
					}
					
					onClicked: function(target_value) {
						domain_game_data.economy.set_min_commodity_storage(commodity, target_value)
					}
				}
				
				CustomSlider {
					id: max_storage_slider
					anchors.left: parent.left
					anchors.right: parent.right
					value: domain_game_data.economy.max_commodity_storages.length > 0 ? domain_game_data.economy.get_max_commodity_storage(commodity) : 0
					max_value: domain_game_data.economy.storage_capacity
					
					onDecremented: {
						if (domain_game_data.economy.get_max_commodity_storage(commodity) === max_storage_slider.min_value) {
							return
						}
						
						domain_game_data.economy.change_max_commodity_storage(commodity, -1)
					}
					
					onIncremented: {
						if (domain_game_data.economy.get_max_commodity_storage(commodity) === max_storage_slider.max_value) {
							return
						}
						
						domain_game_data.economy.change_max_commodity_storage(commodity, 1)
					}
					
					onClicked: function(target_value) {
						domain_game_data.economy.set_max_commodity_storage(commodity, target_value)
					}
				}
			}
		}
	}
	
	Grid {
		id: wealth_convertible_commodity_grid
		anchors.top: storable_commodity_grid.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 4
		spacing: 16 * scale_factor
		
		Repeater {
			model: domain_economy.available_commodities
			
			Item {
				width: 32 * scale_factor
				height: 32 * scale_factor
				visible: !commodity.abstract && commodity.wealth_value !== 0 && output > 0
				
				readonly property var commodity: model.modelData
				readonly property int output: domain_economy.get_commodity_output_int(commodity)
				
				Image {
					id: commodity_icon
					anchors.verticalCenter: parent.verticalCenter
					anchors.horizontalCenter: parent.horizontalCenter
					source: "image://icon/" + commodity.icon.identifier
				}
				
				MouseArea {
					anchors.fill: commodity_icon
					hoverEnabled: true
					
					onEntered: {
						status_text = commodity.name
						if (output > 0) {
							middle_status_text = "Output: " + commodity.value_to_qstring(output)
							middle_status_text += " (" + metternich.defines.wealth_commodity.value_to_qstring(commodity.wealth_value * output) + ")"
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
