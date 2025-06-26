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
		id: transportable_outputs_area
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	Grid {
		id: transportable_outputs_grid
		anchors.top: transportable_outputs_area.top
		anchors.topMargin: 16 * scale_factor
		anchors.bottom: transportable_outputs_area.bottom
		anchors.bottomMargin: 16 * scale_factor
		anchors.horizontalCenter: transportable_outputs_area.horizontalCenter
		columns: 3
		spacing: 32 * scale_factor
		width: (176 * scale_factor + 32 * scale_factor + 16 * scale_factor) * columns + spacing * 2
		
		Repeater {
			model: country_game_data ? country_game_data.transportable_commodity_outputs : []
			
			Item {
				width: commodity_transport_slider.width + 16 * scale_factor + commodity_icon.width
				height: commodity_icon.height
				
				readonly property var commodity: model.modelData.key
				readonly property int transportable_output: model.modelData.value
				property int transported_output: country_game_data.get_transported_commodity_output(commodity)
				
				Image {
					id: commodity_icon
					anchors.verticalCenter: parent.verticalCenter
					anchors.left: parent.left
					source: "image://icon/" + commodity.icon.identifier
					
					MouseArea {
						anchors.fill: parent
						hoverEnabled: true
						
						onEntered: {
							status_text = commodity.name
						}
						
						onExited: {
							if (status_text === commodity.name) {
								status_text = ""
							}
						}
					}
				}
				
				CustomSlider {
					id: commodity_transport_slider
					anchors.verticalCenter: commodity_icon.verticalCenter
					anchors.right: parent.right
					value: transported_output
					max_value: transportable_output
					
					onDecremented: {
						if (transported_output >= 0) {
							country_game_data.change_transported_commodity_output(commodity, -1)
							transported_output = country_game_data.get_transported_commodity_output(commodity)
						}
					}
					
					onIncremented: {
						if (transported_output < transportable_output) {
							country_game_data.change_transported_commodity_output(commodity, 1)
							transported_output = country_game_data.get_transported_commodity_output(commodity)
						}
					}
					
					onClicked: function(target_value) {
						var current_transported_output = transported_output
						
						target_value = Math.min(target_value, transportable_output)
						target_value = Math.max(target_value, 0)
						
						if (target_value !== transported_output) {
							country_game_data.change_transported_commodity_output(commodity, target_value - transported_output)
							transported_output = country_game_data.get_transported_commodity_output(commodity)
						}
						
						status_text = commodity.name + ": " + transported_output + "/" + transportable_output
					}
					
					onEntered: {
						status_text = commodity.name + ": " + transported_output + "/" + transportable_output
					}
					
					onExited: {
						if (status_text === (commodity.name + ": " + transported_output + "/" + transportable_output)) {
							status_text = ""
						}
					}
				}
			}
		}
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
}
