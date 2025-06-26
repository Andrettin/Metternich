import QtQuick
import QtQuick.Controls
import "./dialogs"

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 176 * scale_factor
	clip: true
	
	readonly property var end_turn_button: end_turn_button_internal
	readonly property var province_game_data: selected_province ? selected_province.game_data : null
	readonly property var selected_site_game_data: selected_site ? selected_site.game_data : null
	property bool viewing_population: false
	property bool viewing_settlement_info: false
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	IconButton {
		id: transport_button
		anchors.top: industry_button.top
		anchors.left: parent.left
		anchors.leftMargin: 6 * scale_factor
		icon_identifier: "railroad"
		
		onReleased: {
			menu_stack.push("TransportView.qml", {
				country: metternich.game.player_country
			})
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Transport"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: industry_button
		anchors.top: parent.top
		anchors.topMargin: 6 * scale_factor
		anchors.left: transport_button.right
		anchors.leftMargin: 4 * scale_factor
		icon_identifier: "settlement"
		
		onReleased: {
			menu_stack.push("IndustryView.qml", {
				country: metternich.game.player_country,
				interface_style: map_view.interface_style
			})
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Industry"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: trade_button
		anchors.top: industry_button.top
		anchors.left: industry_button.right
		anchors.leftMargin: 4 * scale_factor
		icon_identifier: "wealth"
		
		onReleased: {
			menu_stack.push("TradeView.qml")
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Trade"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: diplomatic_map_button
		anchors.top: industry_button.top
		anchors.left: trade_button.right
		anchors.leftMargin: 4 * scale_factor
		icon_identifier: "globe"
		
		onReleased: {
			menu_stack.push("DiplomaticView.qml", {
				start_tile_x: map_area_start_x + map_area_tile_width / 2,
				start_tile_y: map_area_start_y + map_area_tile_height / 2
			})
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Diplomatic Map"
			} else {
				status_text = ""
			}
		}
	}
	
	NormalText {
		id: title
		anchors.top: industry_button.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: selected_garrison ? "Garrison" : (
			selected_site ? selected_site.game_data.current_cultural_name
				: (selected_civilian_unit ? selected_civilian_unit.type.name : "")
		)
	}
	
	Image {
		id: icon
		anchors.top: title.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		source: icon_identifier.length > 0 ? ("image://icon/" + icon_identifier) : "image://empty/"
		visible: !selected_garrison && !population_info_text.visible
		
		readonly property string icon_identifier: selected_civilian_unit ? selected_civilian_unit.icon.identifier : (
			selected_site ? (
				(selected_site.settlement && selected_site.game_data.settlement_type !== null) ? "" : (
					(selected_site.game_data.improvement && selected_site.game_data.improvement.ruins) ? "skull" : (selected_site.map_data.resource.icon ? selected_site.map_data.resource.icon.identifier : selected_site.map_data.resource.commodity.icon.identifier)
				)
			) : ""
		)
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			
			onEntered: {
				status_text = title.text
				if (subtitle.text.length > 0) {
					status_text += " (" + subtitle.text + ")"
				}
			}
			
			onExited: {
				status_text = ""
			}
		}
	}
	
	SmallText {
		id: subtitle
		anchors.top: icon.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: (selected_site && !selected_garrison) ? (
			selected_site.settlement ? "" : (
				selected_site.game_data.improvement ? (
					selected_site.game_data.improvement.name
				) : (
					selected_site.map_data.resource ? (selected_site.map_data.resource.natural_wonder ? "Natural Wonder" : selected_site.map_data.resource.name) : ""
				)
			)
		) : (selected_civilian_unit && selected_civilian_unit.character ? selected_civilian_unit.character.full_name : "")
		visible: !population_info_text.visible
	}
	
	ScriptedModifierRow {
		id: scripted_modifier_row
		anchors.top: title.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		scope: selected_site
		visible: selected_site && !selected_garrison && selected_site.game_data.scripted_modifiers.length > 0
	}
	
	Flickable {
		id: portrait_grid_flickable
		anchors.top: scripted_modifier_row.visible ? scripted_modifier_row.bottom : title.bottom
		anchors.topMargin: scripted_modifier_row.visible ? 8 * scale_factor : 16 * scale_factor
		anchors.bottom: end_turn_button_internal.top
		anchors.bottomMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		contentHeight: contentItem.childrenRect.height
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		visible: selected_site !== null && selected_site.settlement && !selected_garrison && !viewing_population && !viewing_settlement_info
		
		Grid {
			id: portrait_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: 2
			spacing: 16 * scale_factor
			
			Repeater {
				model: selected_site !== null && selected_site.settlement ? selected_site_game_data.building_slots : []
				
				PortraitGridItem {
					portrait_identifier: wonder ? wonder.portrait.identifier : (building ? building.portrait.identifier : "building_slot")
					
					readonly property var building_slot: model.modelData
					readonly property var building: building_slot.building
					readonly property var wonder: building_slot.wonder
					
					Image {
						id: under_construction_icon
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
						source: "image://icon/cog"
						visible: building_slot.under_construction_building !== null || building_slot.under_construction_wonder !== null
					}
					
					onClicked: {
						if (building !== null && building_slot.modifier_string.length > 0) {
							modifier_dialog.title = wonder ? wonder.name : building.name
							modifier_dialog.modifier_string = building_slot.modifier_string
							modifier_dialog.open()
						}
					}
					
					onEntered: {
						if (wonder !== null) {
							status_text = wonder.name
						} else if (building !== null) {
							status_text = building.name
						} else {
							status_text = building_slot.type.name + " Slot"
							middle_status_text = ""
						}
					}
					
					onExited: {
						status_text = ""
						middle_status_text = ""
					}
				}
			}
		}
	}
	
	SmallText {
		id: site_info_text
		anchors.top: subtitle.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: format_text(
			selected_site_game_data ? (
				(selected_site_game_data.commodity_outputs.length > 0 ? get_commodity_outputs_string(selected_site_game_data.commodity_outputs) : "")
				+ (selected_site.map_data.resource && selected_site.map_data.resource.commodity && selected_site.game_data.resource_improvement ? ("\nHousing: " + selected_site_game_data.population_unit_count + "/" + selected_site_game_data.housing) : "")
			) : ""
		)
		visible: selected_site && !selected_garrison && !selected_site.settlement && !viewing_population
		
		function get_commodity_outputs_string(commodity_outputs) {
			var str = ""
			
			for (var kv_pair of commodity_outputs) {
				var commodity = kv_pair.key
				var output = kv_pair.value
				
				if (commodity.labor) {
					continue
				}
				
				if (commodity.housing) {
					continue
				}
				
				if (str.length > 0) {
					str += "\n"
				}
				
				str += commodity.name + " Output: " + output
			}
			
			return str
		}
	}
	
	CivilianUnitInfoArea {
		id: civilian_unit_info_area
		anchors.top: subtitle.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.bottom: disband_button.top
		anchors.bottomMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.right: parent.right
		visible: selected_civilian_unit !== null
	}
	
	MilitaryUnitGrid {
		id: military_unit_grid
		anchors.top: title.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.bottom: end_turn_button_internal.top
		anchors.bottomMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.right: parent.right
		visible: selected_province !== null && selected_garrison
	}
	
	SmallText {
		id: population_info_text
		anchors.top: title.bottom
		anchors.topMargin: 12 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
		text: selected_site_game_data ? format_text(
			"Population: " + number_string(selected_site_game_data.population.size)
			 + "\nHousing: " + selected_site_game_data.population_unit_count + "/" + selected_site_game_data.housing
			 + "\nLiteracy: " + selected_site_game_data.population.literacy_rate + "%"
			 + "\nConsciousness: " + selected_site_game_data.population.average_consciousness
			 + "\nMilitancy: " + selected_site_game_data.population.average_militancy
		) : ""
		visible: selected_site !== null && selected_site.game_data.can_have_population() && selected_site.game_data.is_built() && !selected_garrison && viewing_population
	}
	
	Grid {
		id: population_chart_grid
		anchors.top: population_info_text.bottom
		anchors.topMargin: 12 * scale_factor
		anchors.bottom: end_turn_button_internal.top
		anchors.bottomMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 2
		columnSpacing: 16 * scale_factor
		rowSpacing: 8 * scale_factor
		verticalItemAlignment: Grid.AlignVCenter
		visible: population_info_text.visible && selected_site !== null && selected_site.game_data.population.size > 0
		
		Item {
			width: population_type_chart.width
			height: population_type_chart.y + population_type_chart.height
		
			SmallText {
				id: population_type_chart_label
				anchors.horizontalCenter: population_type_chart.horizontalCenter
				text: qsTr("Population Type")
				visible: population_type_chart.visible
			}
			
			PopulationTypeChart {
				id: population_type_chart
				anchors.top: population_type_chart_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				data_source: selected_site_game_data ? selected_site_game_data.population : null
			}
		}
		
		Item {
			width: culture_chart.width
			height: culture_chart.y + culture_chart.height
		
			SmallText {
				id: culture_chart_label
				anchors.horizontalCenter: culture_chart.horizontalCenter
				text: qsTr("Culture")
				visible: culture_chart.visible
			}
			
			CultureChart {
				id: culture_chart
				anchors.top: culture_chart_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				data_source: selected_site_game_data ? selected_site_game_data.population : null
			}
		}
		
		Item {
			width: religion_chart.width
			height: religion_chart.y + religion_chart.height
		
			SmallText {
				id: religion_chart_label
				anchors.horizontalCenter: religion_chart.horizontalCenter
				text: qsTr("Religion")
				visible: religion_chart.visible
			}
			
			ReligionChart {
				id: religion_chart
				anchors.top: religion_chart_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				data_source: selected_site_game_data ? selected_site_game_data.population : null
			}
		}
		
		Item {
			width: ideology_chart.width
			height: ideology_chart.y + ideology_chart.height
		
			SmallText {
				id: ideology_chart_label
				anchors.horizontalCenter: ideology_chart.horizontalCenter
				text: qsTr("Ideology")
				visible: ideology_chart.visible
			}
			
			IdeologyChart {
				id: ideology_chart
				anchors.top: ideology_chart_label.bottom
				anchors.topMargin: 4 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				data_source: selected_site_game_data ? selected_site_game_data.population : null
			}
		}
	}
	
	PortraitButton {
		id: holder_portrait
		anchors.top: selected_site !== null && !selected_site.settlement ? site_info_text.bottom : title.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: holder ? holder.game_data.portrait.identifier : ""
		visible: holder !== null && (viewing_settlement_info || (selected_site !== null && !selected_site.settlement)) && !viewing_population
		circle: true
		
		readonly property var holder: selected_site !== null ? (selected_site.settlement && province_game_data ? province_game_data.governor : selected_site.game_data.landholder) : null
		
		onClicked: {
			character_dialog.character = holder
			character_dialog.modifier_string = holder.game_data.governor ? holder.game_data.get_governor_modifier_qstring(holder.governable_province) : holder.game_data.get_landholder_modifier_qstring(selected_site)
			character_dialog.open()
		}
		
		onHoveredChanged: {
			if (hovered && holder !== null) {
				status_text = holder.game_data.titled_name
			} else {
				status_text = ""
			}
		}
	}
	
	SmallText {
		id: output_info_text
		anchors.top: holder_portrait.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
		text: selected_site_game_data ? format_text(commodity_outputs_to_string(selected_site_game_data.commodity_outputs)) : ""
		visible: selected_site !== null && selected_site.settlement && selected_site.game_data.settlement_type !== null && !selected_garrison && viewing_settlement_info
		
		function commodity_outputs_to_string(commodity_outputs) {
			var str = ""
			
			for (var i = 0; i < commodity_outputs.length; i++) {
				var commodity = commodity_outputs[i].key
				var output = commodity_outputs[i].value
				
				if (commodity.local) {
					continue
				}
				
				if (commodity.labor) {
					continue
				}
				
				if (str.length > 0) {
					str += "\n"
				}
				
				str += commodity.name + (commodity.storable ? " Output" : "") + ": " + output
			}
			
			return str
		}
	}
	
	TextButton {
		id: disband_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: end_turn_button_internal.top
		anchors.bottomMargin: 8 * scale_factor
		text: qsTr("Disband")
		width: 64 * scale_factor
		height: 24 * scale_factor
		visible: selected_civilian_unit !== null
		
		onClicked: {
			selected_civilian_unit.disband()
			selected_civilian_unit = null
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Disband Civilian Unit"
			} else {
				status_text = ""
			}
		}
	}
	
	TextButton {
		id: end_turn_button_internal
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 8 * scale_factor
		text: qsTr("End Turn")
		width: 64 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			metternich.game.do_turn()
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "End Turn"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: population_button
		anchors.right: end_turn_button_internal.left
		anchors.rightMargin: 8 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		icon_identifier: "craftsmen_light_small"
		visible: selected_site !== null && selected_site.game_data.can_have_population() && selected_site.game_data.is_built() && !selected_garrison && !viewing_population
		
		onClicked: {
			viewing_population = true
			viewing_settlement_info = false
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Site Population"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: garrison_button
		anchors.right: end_turn_button_internal.left
		anchors.rightMargin: 8 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		icon_identifier: "crossed_sabers"
		visible: selected_garrison
		
		onReleased: {
			garrison_dialog.open()
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Garrison"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: output_button
		anchors.left: end_turn_button_internal.right
		anchors.leftMargin: 8 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		icon_identifier: "cog"
		visible: selected_site !== null && selected_site.settlement && selected_site.game_data.settlement_type !== null && !selected_garrison && !viewing_settlement_info
		
		onClicked: {
			viewing_settlement_info = true
			viewing_population = false
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "View Settlement Info"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: population_back_button
		anchors.right: end_turn_button_internal.left
		anchors.rightMargin: 8 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		icon_identifier: "settlement"
		visible: selected_site !== null && selected_site.game_data.can_have_population() && selected_site.game_data.is_built() && !selected_garrison && viewing_population
		
		onClicked: {
			viewing_population = false
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = selected_site.settlement ? "Back to Settlement" : "Back to Site"
			} else {
				status_text = ""
			}
		}
	}
	
	IconButton {
		id: output_back_button
		anchors.left: end_turn_button_internal.right
		anchors.leftMargin: 8 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 4 * scale_factor
		icon_identifier: "settlement"
		visible: selected_site !== null && selected_site.settlement && selected_site.game_data.settlement_type !== null && !selected_garrison && viewing_settlement_info
		
		onClicked: {
			viewing_settlement_info = false
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Back to Settlement"
			} else {
				status_text = ""
			}
		}
	}
}
