import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Grid {
	id: background_grid
	rows: Math.ceil(height / (32 * scale_factor)) + 1
	columns: Math.ceil(width / (32 * scale_factor)) + 1
	rowSpacing: 0
	columnSpacing: 0
	
	property string interface_style: "light_wood"
	property int frame_count: 4
	readonly property int tile_count: Math.max(background_grid.rows * background_grid.columns, 0)
	
	Repeater {
		model: tile_count
		
		Image {
			id: background_tile
			source: "image://interface/" + interface_style + "/tiled_background/" + random(frame_count)
		}
	}
}
