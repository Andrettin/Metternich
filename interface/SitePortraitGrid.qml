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
				readonly property var dungeon: site.game_data.dungeon
				readonly property bool is_visit_target: metternich.game.player_country.game_data.visit_target_site === site
				
				Image {
					id: visiting_icon
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.verticalCenter: parent.verticalCenter
					source: "image://icon/flag"
					fillMode: Image.Pad
					visible: is_visit_target
				}
				
				onClicked: {
					if (dungeon !== null) {
						dungeon_dialog.site = site
						dungeon_dialog.open()
					}
				}
				
				onEntered: {
					if (site.game_data.holding_type !== null) {
						status_text = site.game_data.holding_type.name + " of " + site.game_data.current_cultural_name
						if (site.game_data.province.game_data.provincial_capital === site) {
							status_text += " (Provincial Capital)"
						}
					} else if (dungeon !== null) {
						status_text = dungeon.name
						if (dungeon.random) {
							status_text += " of " + site.game_data.current_cultural_name
						}
						status_text += " (Dungeon)"
						if (is_visit_target) {
							status_text += " (Visiting)"
						}
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
