import QtQuick
import QtQuick.Controls
import "./dialogs"

Flickable {
	id: portrait_grid_flickable
	contentHeight: contentItem.childrenRect.height
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	property var sites: []
	
	Grid {
		id: portrait_grid
		anchors.horizontalCenter: parent.horizontalCenter
		columns: 2
		spacing: 16 * scale_factor
		
		Repeater {
			model: sites
			
			PortraitGridItem {
				portrait_identifier: site.game_data.portrait ? site.game_data.portrait.identifier : "building_slot"
				
				readonly property var site: model.modelData
				
				onEntered: {
					if (site.game_data.holding_type !== null) {
						status_text = site.game_data.holding_type.name + " of " + site.game_data.current_cultural_name
					} else if (site.holding_type !== null) {
						status_text = site.game_data.current_cultural_name + " (" + site.holding_type.name + " Slot)"
					} else {
						status_text = site.game_data.current_cultural_name
					}
				}
				
				onExited: {
					status_text = ""
				}
			}
		}
	}
}
