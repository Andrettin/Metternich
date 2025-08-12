import QtQuick
import QtQuick.Controls

Item {
	id: tile
	implicitWidth: tile_size
	implicitHeight: tile_size
	
	Repeater {
		model: border_image_sources
		
		TileImage {
			id: border_image
			tile_image_source: "image://" + modelData
			can_be_subtile: true
			x: ((index === 1 || index === 3) ? 32 * scale_factor : 0) - 16 * scale_factor
			y: (index >= 2 ? 32 * scale_factor : 0) - 16 * scale_factor
		}
	}
}
