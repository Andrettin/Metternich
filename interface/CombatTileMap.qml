import QtQuick
import QtQuick.Controls
import combat_map_grid_model 1.0

TableView {
	id: map
	leftMargin: 0
	rightMargin: 0
	topMargin: 0
	bottomMargin: 0
	contentWidth: tile_size * model.rowCount()
	contentHeight: tile_size * model.columnCount()
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	visible: true
	model: CombatMapGridModel {}
	delegate: CombatTileView {}
	
	function pixel_to_tile_pos(pixel_x, pixel_y) {
		var tile_x = Math.floor(pixel_x / tile_size)
		var tile_y = Math.floor(pixel_y / tile_size)
		
		return Qt.point(tile_x, tile_y)
	}
	
	function center_on_tile(tile_x, tile_y) {
		var pixel_x = tile_x * tile_size - map.width / 2
		var pixel_y = tile_y * tile_size - map.height / 2
		
		map.contentX = Math.min(Math.max(pixel_x, 0), map.contentWidth - map.width)
		map.contentY = Math.min(Math.max(pixel_y, 0), map.contentHeight - map.height)
	}
}
