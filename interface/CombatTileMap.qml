import QtQuick
import QtQuick.Controls
import combat_map_grid_model 1.0

TableView {
	id: map
	leftMargin: 0
	rightMargin: 0
	topMargin: 0
	bottomMargin: 0
	contentWidth: tile_size * model.columnCount()
	contentHeight: tile_size * model.rowCount()
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	visible: true
	model: CombatMapGridModel {}
	delegate: CombatTileView {}
	
	Repeater {
		model: combat ? combat.character_infos : []
		
		Image {
			id: character_icon
			x: tile_pos.x * tile_size + Math.floor((tile_size - character_icon.width) / 2)
			y: tile_pos.y * tile_size + (tile_size - character_icon.height)
			source: "image://icon/" + character.game_data.icon.identifier
			mirror: character_info.defender
			z: 100
			
			readonly property var character_info: model.modelData
			readonly property var character: character_info.character
			readonly property var tile_pos: character_info.tile_pos
			
			Rectangle {
				id: hp_bar_shadow
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 1 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.horizontalCenterOffset: 1 * scale_factor
				color: "black"
				width: 24 * scale_factor
				height: 3 * scale_factor
			}
			
			Rectangle {
				id: hp_bar
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 2 * scale_factor
				anchors.horizontalCenter: parent.horizontalCenter
				color: Qt.rgba(250.0 / 255.0, 5.0 / 255.0, 0.0 / 255.0, 1)
				width: 24 * scale_factor
				height: 3 * scale_factor
				
				Rectangle {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					anchors.left: parent.left
					color: Qt.rgba(95.0 / 255.0, 186.0 / 255.0, 75.0 / 255.0, 1)
					width: Math.floor(parent.width * character.game_data.hit_points / character.game_data.max_hit_points)
				}
			}
		}
	}
	
	function pixel_to_tile_pos(pixel_x, pixel_y) {
		var tile_x = Math.floor(pixel_x / tile_size)
		var tile_y = Math.floor(pixel_y / tile_size)
		
		return Qt.point(tile_x, tile_y)
	}
	
	function tile_to_pixel_pos(tile_x, tile_y) {
		var pixel_x = tile_x * tile_size - map.width / 2
		var pixel_y = tile_y * tile_size - map.height / 2
		
		return Qt.point(pixel_x, pixel_y)
	}
	
	function center_on_tile(tile_x, tile_y) {
		var pixel_x = tile_x * tile_size - map.width / 2
		var pixel_y = tile_y * tile_size - map.height / 2
		
		map.contentX = Math.min(Math.max(pixel_x, 0), map.contentWidth - map.width)
		map.contentY = Math.min(Math.max(pixel_y, 0), map.contentHeight - map.height)
	}
	
	Component.onCompleted: {
		map.center_on_tile(0, Math.floor(model.rowCount() / 2))
	}
}
