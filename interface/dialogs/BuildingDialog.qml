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
			id: built_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: building && building.provincial ? ("Built: " + get_settlement_building_count(building) + "/" + country_game_data.provinces.length) : ""
			visible: false//building && building.provincial
		}
		
		SmallText {
			id: modifier_label
			anchors.horizontalCenter: parent.horizontalCenter
			text: format_text(modifier_string)
			visible: building && modifier_string.length > 0 && !building.warehouse && building_slot.available_education_types.length === 0
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
						
						readonly property string costs_string: civilian_unit_type !== null ? costs_to_string(civilian_unit_type.commodity_costs, undefined, civilian_unit_type.wealth_cost) : ""
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
						
						readonly property string costs_string: transporter_type !== null ? costs_to_string(country_game_data.get_transporter_type_commodity_costs_qvariant_list(transporter_type, 1), undefined, country_game_data.get_transporter_type_wealth_cost(transporter_type, 1)) : ""
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
		
		Grid {
			id: military_unit_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 3
			visible: building && building.recruited_military_unit_categories.length > 0
			spacing: 16 * scale_factor
			
			Repeater {
				model: building ? building.recruited_military_unit_categories : []
				
				Column {
					spacing: 2 * scale_factor
					visible: military_unit_type !== null
					
					readonly property var military_unit_category: model.modelData
					readonly property var military_unit_type: country_game_data.military.get_best_military_unit_category_type(military_unit_category)
					property int military_unit_recruitment_count: military_unit_type !== null ? country_game_data.military.get_military_unit_recruitment_count(military_unit_type) : 0
					
					IconButton {
						id: military_unit_button
						width: 64 * scale_factor + 6 * scale_factor
						height: 64 * scale_factor + 6 * scale_factor
						icon_identifier: military_unit_type !== null ? military_unit_type.icon.identifier : "skull"
						tooltip: tooltip_string.length > 0 ? format_text(small_text(tooltip_string)) : ""
						
						readonly property string costs_string: military_unit_type !== null ? costs_to_string(country_game_data.military.get_military_unit_type_commodity_costs_qvariant_list(military_unit_type, 1), undefined, country_game_data.military.get_military_unit_type_wealth_cost(military_unit_type, 1)) : ""
						readonly property string tooltip_string: costs_string
						
						onClicked: {
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
								if (country_game_data.military.can_decrease_military_unit_recruitment(military_unit_type)) {
									country_game_data.military.decrease_military_unit_recruitment(military_unit_type, true)
									military_unit_recruitment_count = country_game_data.military.get_military_unit_recruitment_count(military_unit_type)
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
								if (country_game_data.military.can_increase_military_unit_recruitment(military_unit_type)) {
									country_game_data.military.increase_military_unit_recruitment(military_unit_type)
									military_unit_recruitment_count = country_game_data.military.get_military_unit_recruitment_count(military_unit_type)
								}
							}
						}
					}
				}
			}
		}
		
		Repeater {
			model: building_slot ? building_slot.available_education_types : []
			
			Item {
				width: Math.max(education_slider.default_width, education_formula_row.width)
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
								text: "$" + number_string(education_type.input_wealth)
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
								var modifier = Math.floor(100 * 100 / (100 + country_game_data.economy.throughput_modifier + country_game_data.economy.get_commodity_throughput_modifier(output_commodity)) - 100)
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
			
			str += "$" + number_string(education_type.input_wealth)
		}
		
		str += " makes 1 " + education_type.output_population_type.name
		
		return str
	}
}
