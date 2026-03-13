import QtQuick
import QtQuick.Controls
import map_grid_model 1.0

TableView {
	id: map
	leftMargin: 0
	rightMargin: 0
	topMargin: 0
	bottomMargin: 0
	contentWidth: tile_size * metternich.map.width
	contentHeight: tile_size * metternich.map.height
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	model: MapGridModel {}
	delegate: TileView {}
	
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
	
	function center_on_country_capital(country) {
		var capital = country.game_data.capital
		
		if (capital === null) {
			return
		}
		
		var capital_game_data = capital.game_data
		var capital_x = capital_game_data.tile_pos.x
		var capital_y = capital_game_data.tile_pos.y
		
		center_on_tile(capital_x, capital_y)
	}
}
