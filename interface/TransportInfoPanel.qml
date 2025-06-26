import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor
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
	
	IndustryCounter {
		id: land_transport_capacity_counter
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.topMargin: 96 * scale_factor
		name: "Available Land Transport Capacity"
		icon_identifier: "road"
		count: country_game_data.land_transport_capacity //FIXME: should be the available capacity, not the total capacity
	}
	
	IndustryCounter {
		id: sea_transport_capacity_counter
		anchors.left: parent.left
		anchors.top: land_transport_capacity_counter.bottom
		anchors.topMargin: 16 * scale_factor
		name: "Available Sea Transport Capacity"
		icon_identifier: "anchor"
		count: country_game_data.sea_transport_capacity //FIXME: should be the available capacity, not the total capacity
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
