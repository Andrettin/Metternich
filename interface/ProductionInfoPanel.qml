import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor + 8 * scale_factor * 2
	clip: true
	
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
	
	Column {
		id: food_consumption_column
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		spacing: 8 * scale_factor
		
		Repeater {
			model: ["starch", "meat", "fruit"]
			
			IndustryCounter {
				name: String(food_type_identifier[0]).toUpperCase() + String(food_type_identifier).slice(1) + " Consumption"
				icon_identifier: get_icon_identifier()
				count_string: domain_game_data.economy.get_food_type_consumption_qstring(food_type_identifier)
				
				readonly property string food_type_identifier: model.modelData
				
				function get_icon_identifier() {
					if (food_type_identifier == "starch") {
						return "grain"
					} else if (food_type_identifier == "meat") {
						return "cattle"
					} else if (food_type_identifier == "fruit") {
						return "fruit"
					} else {
						return ""
					}
				}
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
