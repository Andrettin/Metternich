import QtQuick
import QtQuick.Controls

Rectangle {
	id: minimap_area
	color: interface_background_color
	width: 176 * scale_factor
	height: 128 * scale_factor + 2 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "black"
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	Rectangle {
		id: minimap_borders
		anchors.top: minimap.top
		anchors.topMargin: -1 * scale_factor
		anchors.bottom: minimap.bottom
		anchors.bottomMargin: -1 * scale_factor
		anchors.left: minimap.left
		anchors.leftMargin: -1 * scale_factor
		anchors.right: minimap.right
		anchors.rightMargin: -1 * scale_factor
		color: "gray"
	}
	
	Minimap {
		id: minimap
		anchors.left: parent.left
		anchors.leftMargin: 3 * scale_factor
		anchors.verticalCenter: parent.verticalCenter
	}
	
	Column {
		id: minimap_mode_column
		anchors.left: minimap_borders.right
		anchors.right: parent.right
		anchors.verticalCenter: parent.verticalCenter
		spacing: 4 * scale_factor
		
		IconButton {
			id: political_map_mode_button
			anchors.horizontalCenter: parent.horizontalCenter
			icon_identifier: "flag"
			highlighted: province_map.mode === ProvinceMap.Mode.Political
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Political
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Political Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: site_map_mode_button
			anchors.horizontalCenter: parent.horizontalCenter
			icon_identifier: "settlement"
			highlighted: province_map.mode === ProvinceMap.Mode.Site
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Site
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Site Map Mode"
				} else {
					status_text = ""
				}
			}
		}
		
		IconButton {
			id: cultural_map_mode_button
			anchors.horizontalCenter: parent.horizontalCenter
			icon_identifier: "music"
			highlighted: province_map.mode === ProvinceMap.Mode.Cultural
			
			onClicked: {
				province_map.mode = ProvinceMap.Mode.Cultural
			}
			
			onHoveredChanged: {
				if (hovered) {
					status_text = "Cultural Map Mode"
				} else {
					status_text = ""
				}
			}
		}
	}
}
