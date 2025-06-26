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
	readonly property int capacity: building_slot ? building_slot.capacity : 0
	readonly property string modifier_string: building_slot ? building_slot.country_modifier_string : 0
	
	ExpandBuildingButton {
		id: expand_building_button
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 16 * scale_factor
	}
	
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
			id: built_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: building && building.provincial ? ("Built: " + get_settlement_building_count(building) + "/" + country_game_data.provinces.length) : ""
			visible: building && building.provincial
		}
		
		SmallText {
			id: modifier_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: format_text(modifier_string)
			visible: building && modifier_string.length > 0 && !building.warehouse && building_slot.available_production_types.length === 0 && building_slot.available_education_types.length === 0
		}
		
		SmallText {
			id: storage_capacity_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Storage Capacity: " + number_string(country_game_data.storage_capacity)
			visible: building && building.warehouse
		}
		
		SmallText {
			id: capacity_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Capacity: " + capacity
			visible: capacity > 0 && building_slot.available_production_types.length > 0
		}
		
		Grid {
			id: commodities_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 4
			visible: building && building.warehouse
			
			Repeater {
				model: country_game_data.stored_commodities
				
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
		
		Repeater {
			model: building_slot ? building_slot.available_production_types : []
			
			Item {
				width: Math.max(production_slider.width, production_formula_row.width)
				height: production_slider.height + 8 * scale_factor + production_formula_row.height
				anchors.horizontalCenter: content_column.horizontalCenter
				
				readonly property var production_type: model.modelData
				readonly property var output_commodity: production_type.output_commodity
				property int employed_capacity: building_slot.get_production_type_employed_capacity(production_type)
				property int output_value: building_slot.get_production_type_output(production_type)
				
				Row {
					id: production_formula_row
					anchors.top: parent.top
					anchors.horizontalCenter: parent.horizontalCenter
					
					Repeater {
						model: production_type.input_commodities
						
						Row {
							readonly property var input_commodity: model.modelData.key
							readonly property int input_value: model.modelData.value
							
							Item {
								width: 32 * scale_factor
								height: 32 * scale_factor
								visible: index > 0
								
								NormalText {
									text: "+"
									anchors.verticalCenter: parent.verticalCenter
									anchors.horizontalCenter: parent.horizontalCenter
								}
							}
							
							Repeater {
								model: input_value
								
								Image {
									id: input_commodity_icon
									source: "image://icon/" + input_commodity.icon.identifier
								}
							}
						}
					}
					
					Row {
						visible: production_type.input_wealth !== 0
						
						Item {
							width: 32 * scale_factor
							height: 32 * scale_factor
							
							NormalText {
								text: "+"
								anchors.verticalCenter: parent.verticalCenter
								anchors.horizontalCenter: parent.horizontalCenter
							}
						}
						
						Item {
							width: input_wealth_label.width
							height: 32 * scale_factor
							
							NormalText {
								id: input_wealth_label
								text: "$" + number_string(country_game_data.get_inflated_value(production_type.input_wealth))
								anchors.verticalCenter: parent.verticalCenter
								anchors.horizontalCenter: parent.horizontalCenter
							}
						}
					}
					
					Item {
						width: 32 * scale_factor
						height: 32 * scale_factor
						
						NormalText {
							text: "→"
							anchors.verticalCenter: parent.verticalCenter
							anchors.horizontalCenter: parent.horizontalCenter
						}
					}
					
					Image {
						id: output_commodity_icon
						source: "image://icon/" + output_commodity.icon.identifier
					}
				}
				
				MouseArea {
					anchors.fill: production_formula_row
					hoverEnabled: true
					
					onEntered: {
						status_text = get_production_formula_string(production_type)
					}
					
					onExited: {
						status_text = ""
					}
				}
				
				CustomSlider {
					id: production_slider
					anchors.top: production_formula_row.bottom
					anchors.topMargin: 8 * scale_factor
					anchors.horizontalCenter: parent.horizontalCenter
					width: content_column.width
					value: employed_capacity
					secondary_value: output_value
					max_value: capacity
					
					onDecremented: {
						if (building_slot.can_decrease_production(production_type)) {
							building_slot.decrease_production(production_type, true)
							employed_capacity = building_slot.get_production_type_employed_capacity(production_type)
							output_value = building_slot.get_production_type_output(production_type)
						}
					}
					
					onIncremented: {
						if (building_slot.can_increase_production(production_type)) {
							building_slot.increase_production(production_type)
							employed_capacity = building_slot.get_production_type_employed_capacity(production_type)
							output_value = building_slot.get_production_type_output(production_type)
						}
					}
					
					onClicked: function(target_value) {
						var current_employed_capacity = employed_capacity
						
						if (target_value > current_employed_capacity) {
							while (target_value > current_employed_capacity) {
								if (!building_slot.can_increase_production(production_type)) {
									break
								}
								
								building_slot.increase_production(production_type)
								current_employed_capacity = building_slot.get_production_type_employed_capacity(production_type)
							}
						} else if (target_value < current_employed_capacity) {
							while (target_value < current_employed_capacity) {
								if (!building_slot.can_decrease_production(production_type)) {
									break
								}
								
								building_slot.decrease_production(production_type, true)
								current_employed_capacity = building_slot.get_production_type_employed_capacity(production_type)
							}
						} else {
							return
						}
						
						employed_capacity = current_employed_capacity
						output_value = building_slot.get_production_type_output(production_type)
						update_status_text()
					}
					
					onEntered: {
						update_status_text()
					}
					
					onExited: {
						status_text = ""
					}
					
					function update_status_text() {
						var text = ""
						
						var base_input_commodities = production_type.input_commodities
						var input_commodities = building_slot.get_production_type_inputs(production_type)
						
						for (var i = 0; i < input_commodities.length; i++) {
							var commodity = input_commodities[i].key
							var quantity = input_commodities[i].value
							var base_quantity = base_input_commodities[i].value * employed_capacity
							
							if (text.length > 0) {
								text += " + "
							}
							
							if (quantity !== base_quantity) {
								text += "("
							}
							
							text += base_quantity + " " + commodity.name
							
							if (quantity !== base_quantity) {
								var modifier = Math.floor(100 * 100 / (100 + country_game_data.throughput_modifier + country_game_data.get_commodity_throughput_modifier(output_commodity)) - 100)
								text += " " + (modifier > 0 ? "+" : "-") + " " + Math.abs(modifier) + "% = " + quantity + " " + commodity.name 										
								text += ")"
							}
						}
						
						if (production_type.input_wealth !== 0) {
							var total_input_wealth = building_slot.get_production_type_input_wealth(production_type)
							if (text.length > 0) {
								text += " + "
							}
							
							text += "$" + number_string(total_input_wealth)
						}
						
						text += " → " + employed_capacity + " " + output_commodity.name
						
						if (output_value !== employed_capacity) {
							var modifier = country_game_data.output_modifier + country_game_data.get_commodity_output_modifier(output_commodity)
							if (production_type.industrial) {
								modifier += country_game_data.industrial_output_modifier
							}
							text += " " + (modifier > 0 ? "+" : "-") + " " + Math.abs(modifier) + "% = " + output_value + " " + output_commodity.name
						}
						
						status_text = text
					}
				}
			}
		}
		
		Repeater {
			model: building_slot ? building_slot.available_education_types : []
			
			Item {
				width: Math.max(education_slider.width, education_formula_row.width)
				height: education_slider.height + 8 * scale_factor + education_formula_row.height
				anchors.horizontalCenter: content_column.horizontalCenter
				
				readonly property var education_type: model.modelData
				readonly property var input_population_type: education_type.input_population_type
				readonly property var output_population_type: education_type.output_population_type
				property int employed_capacity: building_slot.get_education_type_employed_capacity(education_type)
				property int output_value: building_slot.get_education_type_output(education_type)
				
				Row {
					id: education_formula_row
					anchors.top: parent.top
					anchors.horizontalCenter: parent.horizontalCenter
					
					Image {
						id: input_population_type_icon
						source: "image://icon/" + input_population_type.small_icon.identifier
					}
					
					Repeater {
						model: education_type.input_commodities
						
						Row {
							readonly property var input_commodity: model.modelData.key
							readonly property int input_value: model.modelData.value
							
							Item {
								width: 32 * scale_factor
								height: 32 * scale_factor
								
								NormalText {
									text: "+"
									anchors.verticalCenter: parent.verticalCenter
									anchors.horizontalCenter: parent.horizontalCenter
								}
							}
							
							Repeater {
								model: input_value
								
								Image {
									id: input_commodity_icon
									source: "image://icon/" + input_commodity.icon.identifier
								}
							}
						}
					}
					
					Row {
						visible: education_type.input_wealth !== 0
						
						Item {
							width: 32 * scale_factor
							height: 32 * scale_factor
							
							NormalText {
								text: "+"
								anchors.verticalCenter: parent.verticalCenter
								anchors.horizontalCenter: parent.horizontalCenter
							}
						}
						
						Item {
							width: input_wealth_label.width
							height: 32 * scale_factor
							
							NormalText {
								id: input_wealth_label
								text: "$" + number_string(country_game_data.get_inflated_value(education_type.input_wealth))
								anchors.verticalCenter: parent.verticalCenter
								anchors.horizontalCenter: parent.horizontalCenter
							}
						}
					}
					
					Item {
						width: 32 * scale_factor
						height: 32 * scale_factor
						
						NormalText {
							text: "→"
							anchors.verticalCenter: parent.verticalCenter
							anchors.horizontalCenter: parent.horizontalCenter
						}
					}
					
					Image {
						id: output_population_type_icon
						source: "image://icon/" + output_population_type.small_icon.identifier
					}
				}
				
				MouseArea {
					anchors.fill: education_formula_row
					hoverEnabled: true
					
					onEntered: {
						status_text = get_education_formula_string(education_type)
					}
					
					onExited: {
						status_text = ""
					}
				}
				
				CustomSlider {
					id: education_slider
					anchors.top: education_formula_row.bottom
					anchors.topMargin: 8 * scale_factor
					anchors.horizontalCenter: parent.horizontalCenter
					width: content_column.width
					value: employed_capacity
					secondary_value: output_value
					max_value: country_game_data.population.get_type_count(input_population_type)
					
					onDecremented: {
						if (building_slot.can_decrease_education(education_type)) {
							building_slot.decrease_education(education_type, true)
							employed_capacity = building_slot.get_education_type_employed_capacity(education_type)
							output_value = building_slot.get_education_type_output(education_type)
						}
					}
					
					onIncremented: {
						if (building_slot.can_increase_education(education_type)) {
							building_slot.increase_education(education_type)
							employed_capacity = building_slot.get_education_type_employed_capacity(education_type)
							output_value = building_slot.get_education_type_output(education_type)
						}
					}
					
					onClicked: function(target_value) {
						var current_employed_capacity = employed_capacity
						
						if (target_value > current_employed_capacity) {
							while (target_value > current_employed_capacity) {
								if (!building_slot.can_increase_education(education_type)) {
									break
								}
								
								building_slot.increase_education(education_type)
								current_employed_capacity = building_slot.get_education_type_employed_capacity(education_type)
							}
						} else if (target_value < current_employed_capacity) {
							while (target_value < current_employed_capacity) {
								if (!building_slot.can_decrease_education(education_type)) {
									break
								}
								
								building_slot.decrease_education(education_type, true)
								current_employed_capacity = building_slot.get_education_type_employed_capacity(education_type)
							}
						} else {
							return
						}
						
						employed_capacity = current_employed_capacity
						output_value = building_slot.get_education_type_output(education_type)
						update_status_text()
					}
					
					onEntered: {
						update_status_text()
					}
					
					onExited: {
						status_text = ""
					}
					
					function update_status_text() {
						var text = ""
						
						text += employed_capacity + " " + input_population_type.name
						
						var base_input_commodities = education_type.input_commodities
						var input_commodities = building_slot.get_education_type_inputs(education_type)
						
						for (var i = 0; i < input_commodities.length; i++) {
							var commodity = input_commodities[i].key
							var quantity = input_commodities[i].value
							var base_quantity = base_input_commodities[i].value * employed_capacity
							
							text += " + "
							
							if (quantity !== base_quantity) {
								text += "("
							}
							
							text += base_quantity + " " + commodity.name
							
							if (quantity !== base_quantity) {
								var modifier = Math.floor(100 * 100 / (100 + country_game_data.throughput_modifier + country_game_data.get_commodity_throughput_modifier(output_commodity)) - 100)
								text += " " + (modifier > 0 ? "+" : "-") + " " + Math.abs(modifier) + "% = " + quantity + " " + commodity.name 										
								text += ")"
							}
						}
						
						if (education_type.input_wealth !== 0) {
							var total_input_wealth = building_slot.get_education_type_input_wealth(education_type)
							if (text.length > 0) {
								text += " + "
							}
							
							text += "$" + number_string(total_input_wealth)
						}
						
						text += " → " + employed_capacity + " " + output_population_type.name
						
						status_text = text
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
	
	function get_settlement_building_count(building) {
		var count = country_game_data.get_settlement_building_count(building)
		
		//also count the best non-capital building immediately below the capital-specific one
		while (building && (building.capital_only || building.provincial_capital_only)) {
			building = building.required_building
			count += country_game_data.get_settlement_building_count(building)
		}
		
		return count
	}
	
	function get_production_formula_string(production_type) {
		var str = ""
		
		var input_commodities = production_type.input_commodities
		
		for (var kv_pair of input_commodities) {
			var commodity = kv_pair.key
			var quantity = kv_pair.value
			
			if (str.length > 0) {
				str += " + "
			}
			
			str += quantity + " " + commodity.name
		}
		
		if (production_type.input_wealth !== 0) {
			if (str.length > 0) {
				str += " + "
			}
			
			str += "$" + number_string(country_game_data.get_inflated_value(production_type.input_wealth))
		}
		
		str += " makes " + production_type.output_value + " " + production_type.output_commodity.name
		
		return str
	}
	
	function get_education_formula_string(education_type) {
		var str = ""
		
		str += "1 " + education_type.input_population_type.name
		
		var input_commodities = education_type.input_commodities
		
		for (var kv_pair of input_commodities) {
			var commodity = kv_pair.key
			var quantity = kv_pair.value
			
			if (str.length > 0) {
				str += " + "
			}
			
			str += quantity + " " + commodity.name
		}
		
		if (education_type.input_wealth !== 0) {
			if (str.length > 0) {
				str += " + "
			}
			
			str += "$" + number_string(country_game_data.get_inflated_value(education_type.input_wealth))
		}
		
		str += " makes 1 " + education_type.output_population_type.name
		
		return str
	}
}
