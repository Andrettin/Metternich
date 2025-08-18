import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: transport_view
	
	property var country: null
	readonly property var country_game_data: country ? country.game_data : null
	property string status_text: ""
	property string middle_status_text: ""
	
	TiledBackground {
		id: background_area
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	RightBar {
		id: right_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: infopanel.right
		anchors.right: right_bar.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
	}
	
	TransportInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	TransportersDialog {
		id: transporters_dialog
	}
}
