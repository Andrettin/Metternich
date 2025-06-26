import QtQuick
import QtQuick.Controls

Flickable {
	id: minimap
	width: 128 * scale_factor
	height: 128 * scale_factor
	contentX: Math.min(Math.max(Math.round(map_area_start_x) / tiles_per_pixel - minimap.width / 2, 0), minimap.contentWidth - minimap.width)
	contentY: Math.min(Math.max(Math.round(map_area_start_y) / tiles_per_pixel - minimap.height / 2, 0), minimap.contentHeight - minimap.height)
	contentWidth: minimap_image.width
	contentHeight: minimap_image.height
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	interactive: false
	
	readonly property real tiles_per_pixel: minimap_image.sourceSize.width / minimap_image.width
	
	Image {
		id: minimap_image
		source: "image://diplomatic_map/minimap/" + metternich.game.turn
		fillMode: Image.Pad
		cache: false
	}
	
	Rectangle {
		id: visible_area_rectangle
		color: "transparent"
		border.color: "white"
		border.width: 1
		x: x_override !== null ? x_override : Math.round(map_area_start_x) / tiles_per_pixel
		y: y_override !== null ? y_override : Math.round(map_area_start_y) / tiles_per_pixel
		width: map_area_tile_width / tiles_per_pixel
		height: map_area_tile_height / tiles_per_pixel
		
		property var x_override: null
		property var y_override: null
	}
	
	MouseArea {
		anchors.fill: parent
		
		onPositionChanged: function(mouse) {
			if (!pos_in_visible_area(mouse.x, mouse.y)) {
				visible_area_rectangle.x_override = null
				visible_area_rectangle.y_override = null
				return
			}
			
			visible_area_rectangle.x_override = mouse.x - visible_area_rectangle.width / 2
			visible_area_rectangle.y_override = mouse.y - visible_area_rectangle.height / 2
		}
		
		onPressed: function(mouse) {
			if (!pos_in_visible_area(mouse.x, mouse.y)) {
				visible_area_rectangle.x_override = null
				visible_area_rectangle.y_override = null
				return
			}
			
			visible_area_rectangle.x_override = mouse.x - visible_area_rectangle.width / 2
			visible_area_rectangle.y_override = mouse.y - visible_area_rectangle.height / 2
		}
		
		onExited: {
			visible_area_rectangle.x_override = null
			visible_area_rectangle.y_override = null
		}
		
		onClicked: function(mouse) {
			var pixel_x = mouse.x
			var pixel_y = mouse.y
			
			if (!pos_in_visible_area(pixel_x, pixel_y)) {
				visible_area_rectangle.x_override = null
				visible_area_rectangle.y_override = null
				return
			}
			
			center(pixel_x, pixel_y)
			visible_area_rectangle.x_override = null
			visible_area_rectangle.y_override = null
		}
		
		function center(pixel_x, pixel_y) {
			var tile_x = Math.floor(pixel_x * tiles_per_pixel)
			var tile_y = Math.floor(pixel_y * tiles_per_pixel)
			map.center_on_tile(tile_x, tile_y)
		}
		
		function pos_in_visible_area(pixel_x, pixel_y) {
			return pixel_x >= minimap.contentX && pixel_y >= minimap.contentY && pixel_x < (minimap.contentX + minimap.width) && pixel_y < (minimap.contentY + minimap.height)
		}
	}
}
