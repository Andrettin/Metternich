import QtQuick
import QtQuick.Controls

Rectangle {
	id: top_bar
	color: interface_background_color
	height: 16 * scale_factor
	clip: true
	
	readonly property var domain: metternich.game.player_country
	readonly property var stored_commodities: domain ? domain.game_data.economy.stored_commodities : []
	readonly property var commodity_storage_capacities: domain ? domain.game_data.economy.commodity_storage_capacities : []
	readonly property var regency_commodity: metternich.get_commodity("regency")
	readonly property var piety_commodity: metternich.get_commodity("piety")
	readonly property var top_bar_commodities: [metternich.defines.wealth_commodity, regency_commodity, piety_commodity]
	property bool commodities_visible: domain ? true : false
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		height: 1 * scale_factor
		z: 1 //draw on top of everything else
	}
	
	MouseArea {
		id: top_bar_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		onContainsMouseChanged: {
			if (typeof status_text !== 'undefined') {
				if (containsMouse) {
					status_text = ""
				}
			}
		}
	}
	
	SmallText {
		id: date_label
		text: metternich.game.date_string
		anchors.top: parent.top
		anchors.topMargin: 1 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 16 * scale_factor
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			onEntered: {
				if (typeof status_text !== 'undefined') {
					status_text = "Current Season and Year"
				}
			}
			onExited: {
				if (typeof status_text !== 'undefined') {
					status_text = ""
				}
			}
		}
	}
	
	Row {
		id: commodities_row
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: date_label.left
		anchors.leftMargin: 128 * scale_factor
		anchors.right: parent.right
		visible: commodities_visible
		
		Repeater {
			model: top_bar_commodities
			
			Item {
				width: commodity_icon.width + (2 + 96) * scale_factor
				height: commodities_row.height
				visible: commodity === metternich.defines.wealth_commodity || commodity === metternich.defines.regency_commodity || commodity_value > 0
				
				readonly property var commodity: model.modelData
				readonly property var commodity_value: commodity && domain !== null && stored_commodities.length > 0 ? domain.game_data.economy.get_stored_commodity(commodity) : 0 //refer to the stored commodities to ensure the counter is updated when the storage value for this commodity changes
				
				Image {
					id: commodity_icon
					source: commodity ? ("image://icon/" + commodity.tiny_icon.identifier) : "image://empty/"
					anchors.top: parent.top
					anchors.topMargin: 3 * scale_factor
					anchors.left: parent.left
					visible: commodity && commodity.enabled
				}

				SmallText {
					id: commodity_label
					text: commodity ? commodity.value_to_qstring(commodity_value) : ""
					anchors.top: parent.top
					anchors.topMargin: 1 * scale_factor
					anchors.left: commodity_icon.right
					anchors.leftMargin: 2 * scale_factor
					visible: commodity && commodity.enabled
				}

				MouseArea {
					anchors.top: commodity_icon.top
					anchors.bottom: commodity_icon.bottom
					anchors.left: commodity_icon.left
					anchors.right: commodity_label.right
					hoverEnabled: true
					enabled: commodity && commodity.enabled
					
					readonly property bool is_wealth: commodity === metternich.defines.wealth_commodity
					readonly property var commodity_unit: commodity ? commodity.get_unit(commodity_value) : null
					readonly property int min_income: is_wealth && domain ? domain.game_data.min_income : 0
					readonly property int max_income: is_wealth && domain ? domain.game_data.max_income : 0
					readonly property string income_string: get_income_range_string(min_income, max_income)
					readonly property int maintenance_cost: is_wealth && domain ? domain.game_data.maintenance_cost : 0
					readonly property int commodity_storage_capacity: domain && commodity && commodity.special_storage_capacity && commodity_storage_capacities.length > 0 ? domain.game_data.economy.get_commodity_storage_capacity(commodity) : 0
					readonly property string commodity_status_text: commodity ? ((commodity_unit && is_wealth ? get_plural_form(commodity_unit.name) : commodity.name)
						+ (is_wealth && income_string.length > 0 ? format_text("\t\tIncome: " + income_string) : "")
						+ (is_wealth ? format_text("\t\tMaintenance Cost: " + commodity.value_to_qstring(maintenance_cost)) : "")
						+ (commodity.special_storage_capacity ? format_text("\t\tMaximum: " + commodity.value_to_qstring(commodity_storage_capacity)) : "")
						+ (commodity_unit ? format_text("\t\t" + commodity.get_units_tooltip()) : "")) : ""
					
					onEntered: {
						if (typeof status_text !== 'undefined') {
							status_text = commodity_status_text
						}
					}
					onExited: {
						if (typeof status_text !== 'undefined') {
							status_text = ""
						}
					}
					onCommodity_status_textChanged: {
						if (containsMouse && typeof status_text !== 'undefined') {
							status_text = commodity_status_text
						}
					}
				}
			}
		}
	}
}
