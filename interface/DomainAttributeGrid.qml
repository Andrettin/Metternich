import QtQuick
import QtQuick.Controls

Grid {
	id: domain_attribute_grid
	columns: 6
	columnSpacing: 4 * scale_factor
	rowSpacing: 4 * scale_factor
	verticalItemAlignment: Grid.AlignVCenter
	
	property var domain: null
	
	Repeater {
		model: domain && domain.game_data ? domain.game_data.attribute_values : []
		
		Item {
			width: attribute_icon.width + (16 * scale_factor) + 4 * scale_factor
			height: Math.max(attribute_icon.height, attribute_label.height)
			
			readonly property var attribute: model.modelData.key
			readonly property int attribute_value: model.modelData.value
		
			Image {
				id: attribute_icon
				source: attribute ? ("image://icon/" + attribute.tiny_icon.identifier) : "image://empty/"
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: parent.left
			}

			SmallText {
				id: attribute_label
				text: number_string(attribute_value)
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: attribute_icon.right
				anchors.leftMargin: 4 * scale_factor
			}

			MouseArea {
				anchors.top: attribute_icon.top
				anchors.bottom: attribute_icon.bottom
				anchors.left: attribute_icon.left
				anchors.right: attribute_label.right
				hoverEnabled: true
				
				onEntered: {
					status_text = attribute.name
				}
				onExited: {
					status_text = ""
				}
			}
		}
	}
	
	Item {
		width: consumption_icon.width + (16 * scale_factor) + 4 * scale_factor
		height: Math.max(consumption_icon.height, consumption_label.height)
		visible: domain && domain.game_data && domain.game_data.consumption !== 0
		
		Image {
			id: consumption_icon
			source: "image://icon/dungeon"
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
		}

		SmallText {
			id: consumption_label
			text: domain && domain.game_data ? number_string(domain.game_data.consumption) : ""
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: consumption_icon.right
			anchors.leftMargin: 4 * scale_factor
		}

		MouseArea {
			anchors.top: consumption_icon.top
			anchors.bottom: consumption_icon.bottom
			anchors.left: consumption_icon.left
			anchors.right: consumption_label.right
			hoverEnabled: true
			
			onEntered: {
				status_text = "Consumption"
			}
			onExited: {
				status_text = ""
			}
		}
	}
	
	Item {
		width: unrest_icon.width + (16 * scale_factor) + 4 * scale_factor
		height: Math.max(unrest_icon.height, unrest_label.height)
		visible: domain && domain.game_data && domain.game_data.unrest !== 0
		
		Image {
			id: unrest_icon
			source: "image://icon/war"
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: parent.left
		}

		SmallText {
			id: unrest_label
			text: domain && domain.game_data ? number_string(domain.game_data.unrest) : ""
			anchors.verticalCenter: parent.verticalCenter
			anchors.left: unrest_icon.right
			anchors.leftMargin: 4 * scale_factor
		}

		MouseArea {
			anchors.top: unrest_icon.top
			anchors.bottom: unrest_icon.bottom
			anchors.left: unrest_icon.left
			anchors.right: unrest_label.right
			hoverEnabled: true
			
			onEntered: {
				status_text = "Unrest"
			}
			onExited: {
				status_text = ""
			}
		}
	}
}
