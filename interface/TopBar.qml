import QtQuick
import QtQuick.Controls

Rectangle {
	id: top_bar
	color: interface_background_color
	height: 16 * scale_factor
	clip: true
	
	readonly property var stored_commodities: metternich.game.player_country.game_data.economy.stored_commodities
	readonly property var regency_commodity: metternich.get_commodity("regency")
	readonly property var top_bar_commodities: [metternich.defines.wealth_commodity, regency_commodity]
	property bool commodities_visible: true
	
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
				status_text = "Current Season and Year"
			}
			onExited: {
				status_text = ""
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
				
				readonly property var commodity: model.modelData
				readonly property var commodity_value: stored_commodities.length > 0 ? metternich.game.player_country.game_data.economy.get_stored_commodity(commodity) : 0 //refer to the stored commodities to ensure the counter is updated when the storage value for this commodity changes
				
				Image {
					id: commodity_icon
					source: "image://icon/" + commodity.tiny_icon.identifier
					anchors.top: parent.top
					anchors.topMargin: 3 * scale_factor
					anchors.left: parent.left
					visible: commodity.enabled
				}

				SmallText {
					id: commodity_label
					text: commodity.value_to_qstring(commodity_value)
					anchors.top: parent.top
					anchors.topMargin: 1 * scale_factor
					anchors.left: commodity_icon.right
					anchors.leftMargin: 2 * scale_factor
					visible: commodity.enabled
				}

				MouseArea {
					anchors.top: commodity_icon.top
					anchors.bottom: commodity_icon.bottom
					anchors.left: commodity_icon.left
					anchors.right: commodity_label.right
					hoverEnabled: true
					enabled: commodity.enabled
					
					readonly property bool is_wealth: commodity === metternich.defines.wealth_commodity
					readonly property var commodity_unit: commodity.get_unit(commodity_value)
					readonly property int min_income: is_wealth ? metternich.game.player_country.game_data.min_income : 0
					readonly property int max_income: is_wealth ? metternich.game.player_country.game_data.max_income : 0
					readonly property string income_string: get_income_range_string(min_income, max_income)
					readonly property int maintenance_cost: is_wealth ? metternich.game.player_country.game_data.maintenance_cost : 0
					readonly property string commodity_status_text: (commodity_unit ? get_plural_form(commodity_unit.name) : commodity.name)
						+ (is_wealth && income_string.length > 0 ? format_text("\t\tIncome: " + income_string) : "")
						+ (is_wealth ? format_text("\t\tMaintenance Cost: " + commodity.value_to_qstring(maintenance_cost)) : "")
						+ (commodity_unit ? format_text("\t\t" + commodity.get_units_tooltip()) : "")
					
					onEntered: {
						if (status_text !== undefined) {
							status_text = commodity_status_text
						}
					}
					onExited: {
						if (status_text !== undefined) {
							status_text = ""
						}
					}
					onCommodity_status_textChanged: {
						if (containsMouse) {
							status_text = commodity_status_text
						}
					}
				}
			}
		}
	}
}
