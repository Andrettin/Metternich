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
					} else {
						selected_civilian_unit = null
						selected_site = site
						selected_province = null
					}
				}
				
				onEntered: {
					status_text = site.game_data.display_text
				}
				
				onExited: {
					status_text = ""
				}
			}
		}
	}
}
