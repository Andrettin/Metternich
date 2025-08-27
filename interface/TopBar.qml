import QtQuick
import QtQuick.Controls

Rectangle {
	id: top_bar
	color: interface_background_color
	height: 16 * scale_factor
	clip: true
	
	property bool prestige_visible: true
	readonly property var stored_commodities: metternich.game.player_country.game_data.economy.stored_commodities
	readonly property var wealth_value: metternich.game.player_country.game_data.economy.stored_commodities.length > 0 ? metternich.game.player_country.game_data.economy.get_stored_commodity(metternich.defines.wealth_commodity) : 0 //refer to the stored commodities to ensure the counter is updated when wealth changes
	readonly property var wealth_unit: metternich.defines.wealth_commodity.get_unit(wealth_value)
	
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
	
	Image {
		id: wealth_icon
		source: "image://icon/trade_consulate"
		anchors.top: parent.top
		anchors.topMargin: 3 * scale_factor
		anchors.left: date_label.left
		anchors.leftMargin: 128 * scale_factor
	}

	SmallText {
		id: wealth_label
		text: metternich.defines.wealth_commodity.value_to_qstring(wealth_value)
		anchors.top: parent.top
		anchors.topMargin: 1 * scale_factor
		anchors.left: wealth_icon.right
		anchors.leftMargin: 4 * scale_factor
	}

	MouseArea {
		anchors.top: wealth_icon.top
		anchors.bottom: wealth_icon.bottom
		anchors.left: wealth_icon.left
		anchors.right: wealth_label.right
		hoverEnabled: true
		
		readonly property int min_income: metternich.game.player_country.game_data.min_income
		readonly property int max_income: metternich.game.player_country.game_data.max_income
		readonly property var income_unit: metternich.defines.wealth_commodity.get_unit(max_income)
		readonly property int income_unit_value: metternich.defines.wealth_commodity.get_unit_value(income_unit)
		readonly property int maintenance_cost: metternich.game.player_country.game_data.maintenance_cost
		readonly property string wealth_status_text: get_plural_form(wealth_unit.name)
			+ format_text("\t\tIncome: " + number_string(Math.floor(min_income / income_unit_value)) + "-" + number_string(Math.floor(max_income / income_unit_value)) + " " + income_unit.suffix)
			+ format_text("\t\tMaintenance Cost: " + metternich.defines.wealth_commodity.value_to_qstring(maintenance_cost))
			+ format_text("\t\t" + metternich.defines.wealth_commodity.get_units_tooltip())
		
		onEntered: {
			if (status_text !== undefined) {
				status_text = wealth_status_text
			}
		}
		onExited: {
			if (status_text !== undefined) {
				status_text = ""
			}
		}
		onWealth_status_textChanged: {
			if (containsMouse) {
				status_text = wealth_status_text
			}
		}
	}
	
	Image {
		id: prestige_icon
		source: "image://icon/non_aggression_pact_shield"
		anchors.top: parent.top
		anchors.topMargin: 3 * scale_factor
		anchors.left: wealth_label.left
		anchors.leftMargin: 96 * scale_factor
		visible: prestige_visible && metternich.defines.prestige_commodity.enabled
	}

	SmallText {
		id: prestige_label
		text: metternich.game.player_country.game_data.economy.stored_commodities.length > 0 ? number_string(metternich.game.player_country.game_data.economy.get_stored_commodity(metternich.defines.prestige_commodity)) : 0 //refer to the stored commodities to ensure the counter is updated when prestige changes
		anchors.top: parent.top
		anchors.topMargin: 1 * scale_factor
		anchors.left: prestige_icon.right
		anchors.leftMargin: 2 * scale_factor
		visible: prestige_visible && metternich.defines.prestige_commodity.enabled
	}

	MouseArea {
		anchors.top: prestige_icon.top
		anchors.bottom: prestige_icon.bottom
		anchors.left: prestige_icon.left
		anchors.right: prestige_label.right
		hoverEnabled: true
		enabled: prestige_visible && metternich.defines.prestige_commodity.enabled
		onEntered: {
			if (status_text !== undefined) {
				status_text = "Prestige"
			}
		}
		onExited: {
			if (status_text !== undefined) {
				status_text = ""
			}
		}
	}
}
