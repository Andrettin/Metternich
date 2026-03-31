import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: dungeon_dialog
	title: "Explore Dungeon"
	width: Math.max(portrait_button_width * dungeon_grid.columns + dungeon_grid.spacing * (dungeon_grid.columns - 1), close_button.width) + 8 * scale_factor * 2
	height: close_button.y + close_button.height + 8 * scale_factor
	
	property var dungeon_sites: []
	readonly property int portrait_button_width: 64 * scale_factor + 2 * scale_factor
	readonly property int portrait_button_height: 64 * scale_factor + 2 * scale_factor
	
	Flickable {
		id: dungeon_grid_view
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: Math.min(portrait_button_height * 4 + dungeon_grid.spacing * 3, contentHeight)
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Grid {
			id: dungeon_grid
			anchors.horizontalCenter: parent.horizontalCenter
			columns: Math.min(3, dungeon_sites.length)
			spacing: 8 * scale_factor
			
			Repeater {
				model: dungeon_sites
				
				PortraitButton {
					id: dungeon_portrait
					portrait_identifier: dungeon.portrait.identifier
					
					readonly property var dungeon_site: model.modelData
					readonly property var dungeon: dungeon_site.game_data.dungeon
					
					onClicked: {
						if (metternich.selected_military_units.length > 0) {
							metternich.move_selected_military_units_to(dungeon_site.map_data.tile_pos)
							metternich.clear_selected_military_units()
							dungeon_dialog.close()
						} else {
							metternich.defines.error_sound.play()
						}
					}
					
					onHoveredChanged: {
						if (typeof status_text !== 'undefined') {
							if (hovered) {
								status_text = dungeon_site.game_data.display_text
							} else {
								status_text = ""
							}
						}
					}
				}
			}
		}
	}
	
	TextButton {
		id: close_button
		anchors.top: dungeon_grid_view.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: "Close"
		onClicked: {
			dungeon_dialog.close()
		}
	}
}
