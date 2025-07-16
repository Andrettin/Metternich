import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor + 8 * scale_factor * 2
	clip: true
	
	readonly property var advisor_commodity: metternich.defines.advisor_commodity
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	IndustryCounter {
		id: advisor_commodity_counter
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		name: advisor_commodity.name
		icon_identifier: advisor_commodity.icon.identifier
		count: country_game_data.economy.stored_commodities.length > 0 ? country_game_data.economy.get_stored_commodity(advisor_commodity) : 0
	}
	
	SmallText {
		id: advisor_cost_label
		text: "(" + number_string(country_game_data.advisor_cost) + ")"
		anchors.top: advisor_commodity_counter.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		visible: next_advisor_portrait.visible
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			
			onEntered: {
				status_text = advisor_commodity.name + " required for the next advisor"
			}
			
			onExited: {
				status_text = ""
			}
		}
	}
	
	SmallText {
		id: next_advisor_label
		text: "Next Advisor"
		anchors.top: advisor_cost_label.bottom
		anchors.topMargin: 32 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		visible: next_advisor_portrait.visible
	}
	
	PortraitButton {
		id: next_advisor_portrait
		anchors.top: next_advisor_label.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: country_game_data.next_advisor ? country_game_data.next_advisor.game_data.portrait.identifier : ""
		visible: country_game_data.next_advisor !== null
		
		onClicked: {
			character_dialog.office = null
			character_dialog.character = country_game_data.next_advisor
			character_dialog.modifier_string = country_game_data.next_advisor.game_data.get_advisor_effects_string(country)
			character_dialog.open()
		}
		
		onHoveredChanged: {
			if (hovered && country_game_data.next_advisor) {
				status_text = country_game_data.next_advisor.full_name
			} else {
				status_text = ""
			}
		}
	}
	
	TextButton {
		id: back_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Back")
		width: 48 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
}
