import QtQuick
import QtQuick.Controls

Rectangle {
	id: top_bar
	color: interface_background_color
	height: 16 * scale_factor
	clip: true
	
	property bool prestige_visible: true
	readonly property var stored_commodities: metternich.game.player_country.game_data.stored_commodities
	
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
	
	SmallText {
		id: wealth_label
		text: (metternich.game.player_country.game_data.wealth < 0 ? "-" : "") + "$" + number_string(Math.abs(metternich.game.player_country.game_data.wealth))
		anchors.top: parent.top
		anchors.topMargin: 1 * scale_factor
		anchors.left: date_label.left
		anchors.leftMargin: 128 * scale_factor
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			onEntered: {
				status_text = "Wealth"
			}
			onExited: {
				status_text = ""
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
		text: metternich.game.player_country.game_data.stored_commodities.length > 0 ? number_string(metternich.game.player_country.game_data.get_stored_commodity(metternich.defines.prestige_commodity)) : 0 //refer to the stored commodities to ensure the counter is updated when prestige changes
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
			status_text = "Prestige"
		}
		onExited: {
			status_text = ""
		}
	}
}
